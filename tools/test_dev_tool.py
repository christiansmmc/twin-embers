"""dev.py's incremental sync of build/dev and build/host-tests, and its test command (no git, network or MSYS)."""
import argparse
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import dev  # noqa: E402

LOCK = {'commit': 'c' * 40, 'repository': 'x'}
RECIPES = {'patches/pokefirered/0001.json': 'r1'}


class DevSync(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name) / 'root'
        self.dest = Path(tmp.name) / 'dest'
        (self.root / 'tools').mkdir(parents=True)
        (self.root / 'tools/a.py').write_bytes(b'new')
        (self.root / 'upstream.lock').write_text('[pokefirered]\ncommit = "%s"\nrepository = "x"\n' % LOCK['commit'])
        (self.root / 'patches/pokefirered').mkdir(parents=True)
        self.dest.mkdir()
        (self.dest / 'tools').mkdir()
        (self.dest / 'tools/a.py').write_bytes(b'old')
        self.run_mock = self.enter(mock.patch.object(dev.subprocess, 'run'))
        self.enter(mock.patch.object(dev, 'ROOT', self.root))
        self.enter(mock.patch.object(dev, 'inputs', return_value={'tools/a.py': 'new', **RECIPES}))
        self.remove_tree = self.enter(mock.patch.object(dev, 'remove_tree'))

    def enter(self, patcher):
        started = patcher.start()
        self.addCleanup(patcher.stop)
        return started

    def test_sync_writes_only_changed_files(self):
        (self.dest / '.dev-sync.json').write_text(json.dumps(
            {'commit': LOCK['commit'], 'recipes': RECIPES, 'patched': [], 'repo': {'tools/a.py': 'old'}}))
        dev.sync(self.dest)
        self.assertEqual((self.dest / 'tools/a.py').read_bytes(), (self.root / 'tools/a.py').read_bytes())
        self.run_mock.assert_not_called()
        self.assertEqual(json.loads((self.dest / '.dev-sync.json').read_text())['repo'], {'tools/a.py': 'new'})

    def test_sync_adopts_a_bootstrapped_tree(self):
        (self.dest / '.twin-bootstrap.json').write_text(json.dumps({'inputs': {'tools/a.py': 'new', **RECIPES}}))
        self.run_mock.return_value = mock.Mock(returncode=0, stdout=LOCK['commit'] + '\n')
        dev.sync(self.dest)
        self.assertTrue((self.dest / '.dev-sync.json').exists())
        self.remove_tree.assert_not_called()
        self.assertTrue(all('bootstrap.py' not in ' '.join(map(str, c.args[0])) for c in self.run_mock.call_args_list))

    def test_sync_rebuilds_another_commit_with_remove_tree(self):
        (self.dest / '.dev-sync.json').write_text(json.dumps(
            {'commit': 'd' * 40, 'recipes': RECIPES, 'patched': [], 'repo': {}}))
        with mock.patch.object(dev.shutil, 'rmtree', side_effect=PermissionError):
            dev.sync(self.dest)
        self.remove_tree.assert_called_once_with(self.dest)
        command = self.run_mock.call_args.args[0]
        self.assertIn('--dir', command)
        self.assertIn(str(self.dest), command)


class DevTest(unittest.TestCase):
    def test_test_syncs_the_host_tree_and_filters_suites(self):
        with tempfile.TemporaryDirectory() as tmp:
            kit = Path(tmp)
            (kit / 'msys2/usr/bin').mkdir(parents=True)
            (kit / 'msys2/usr/bin/bash.exe').write_bytes(b'')
            (kit / 'msys2/usr/bin/gcc.exe').write_bytes(b'')
            commands = []

            def fake_run(command, **_options):
                commands.append(command)
                return mock.Mock(returncode=0)

            with mock.patch.object(dev, 'DEVKITPRO', kit), \
                    mock.patch.object(dev.subprocess, 'run', side_effect=fake_run), \
                    mock.patch.object(dev, 'sync') as sync:
                dev.test(argparse.Namespace(suites=[]))
                sync.assert_called_once_with(dev.ROOT / 'build/host-tests')
                bash = [c for c in commands if len(c) == 3 and c[1] == '-lc']
                self.assertTrue(any('verify-host' in c[-1] for c in bash))
                commands.clear()
                sync.reset_mock()
                dev.test(argparse.Namespace(suites=['test_x']))
                sync.assert_not_called()
                bash = [c for c in commands if len(c) == 3 and c[1] == '-lc']
                self.assertIn('-m unittest test_x', bash[-1][-1])
                self.assertFalse(any('verify-host' in c[-1] for c in bash))


if __name__ == '__main__':
    unittest.main()
