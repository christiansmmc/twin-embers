# Brief para o Claude Design: tela de baixo do Classic FireRed 3DS

Cole este texto no Claude Design antes de desenhar. Ele descreve a plataforma, as peças que existem e o que cada
tela precisa ter. O layout (o que vai onde, tamanhos, espaços) é seu.

## A ideia

A tela de baixo do 3DS deve parecer uma **extensão do Pokémon FireRed**, e não um app moderno ao lado do jogo.
Referência de espaço e simplicidade: a tela inicial do port do NachoGonzalez2001 (painéis claros, tela inteira bem
aproveitada, nada apertado). O material é o do próprio FireRed: as molduras de janela do jogo, a fonte do jogo, a
setinha de seleção, os ícones de item e de Pokémon, nos pixels do jogo.

## Plataforma

- Tela de baixo do 3DS: **320 x 240 pixels**, toque com caneta (a caneta é precisa).
- Tudo é desenhado pixel a pixel pelo próprio jogo. Não existe HTML, CSS, fonte do sistema, transparência
  suave, desfoque nem animação de interface em tempo de execução.
- Desenhe em **320 x 240 a 1x**. Para enxergar melhor, visualize ampliado 2x ou 3x **sem suavização** (pixels
  quadrados).
- **Grade de 8 px**: as molduras do FireRed são feitas de blocos de 8 x 8. Painéis com moldura têm largura e altura
  em múltiplos de 8. Fora isso, posições livres.

## Peças do jogo (use blocos marcados, nunca cole arte do jogo)

A arte do jogo não pode ir para o mockup: ela só existe no pack que o jogador gera a partir do próprio ROM, e o jogo
a carrega sozinho. No Claude Design, desenhe cada peça como um bloco cinza com o nome dela e o tamanho certo.

| Bloco marcado | Tamanho real | Como é no jogo |
|---|---|---|
| MOLDURA DIÁLOGO | painel de no mínimo 40 x 32 px | A borda da caixa de diálogo do FireRed (a que aparece em cima quando alguém fala). Borda visível de ~5 px em cima e embaixo e ~11 px nos lados, cantos levemente arredondados, interior branco. Use em **painéis de mensagem e de descrição**. |
| MOLDURA FRAME | anel de 8 px em volta do conteúdo | A moldura que o jogador escolhe em OPTIONS → FRAME (10 estilos). Interior branco. Use em **listas e menus**. |
| SETA ▶ | 8 x 14 px | A setinha dos menus do FireRed, cinza escuro como o texto. Marca a opção escolhida; o texto começa 8 px à direita dela. Sempre visível na opção atual. |
| ÍCONE ITEM 24 | 24 x 24 px | Ícone de item do jogo (Poké Ball, mochila...). Pode ser 48 x 48 (2x exato), nunca outro tamanho. |
| ÍCONE POKÉMON 32 | 32 x 32 px | O ícone pequeno do Pokémon, como no menu do time. |
| MAPA 192x128 | 192 x 128 px | O mapa da região do FireRed, inteiro, em tamanho real. Os destinos de voo são células de 8 x 8. |

## Texto

- Fonte do jogo. Linha de **14 px**; maiúsculas com **9 px** de altura; letras até **10 px** de largura (a maioria
  tem 5 a 6 px). Existe uma fonte menor, com linha de **13 px**, para quando faltar espaço.
- Cor: texto cinza escuro com uma sombra cinza clara, sobre branco. No mockup: **#626262** (texto),
  **#D5D5CD** (sombra, 1 px à direita e abaixo), **#FFFFFF** (fundo).
- Nomes do jogo podem ser longos (lugares como "SEAFOAM ISLANDS", nome do jogador com até 7 letras): deixe espaço ou
  aceite que o texto seja cortado no fim do painel.

## Toque

- Botões com no mínimo **24 x 24 px**.
- Linhas de lista com no mínimo **16 px** de altura.

## Como as telas funcionam

- **Cada tela ocupa a tela inteira (320 x 240).** A tela inicial (HOME) tem o menu; cada item abre a sua tela, e
  BACK (ou o botão B) volta para a inicial.
- A tela de baixo não escurece à noite.

## O que não pode ter

Degradê, brilho, sombra projetada, cantos muito arredondados, chaves liga/desliga estilo celular, partículas
animadas, imagem ampliada por um fator quebrado (só 1x ou 2x exatos).

## O que cada tela precisa ter (fase 1)

### HOME (tela inicial, fora de batalha)

- O mapa da região (MAPA 192x128) com a posição do jogador, num painel com moldura cuja **faixa de baixo, mais
  grossa, mostra o nome do lugar à esquerda e a região à direita** (como na tela do port do Nacho).
- Tocar duas vezes num lugar do mapa voa para lá, quando o VOO já está disponível.
- Uma mensagem de contexto (por exemplo "Tap a destination").
- O menu: POKéDEX (quando o jogador já tem), POKéMON, BAG, o Cartão de Treinador (com o nome do jogador como rótulo),
  SAVE, OPTIONS.
- Os atalhos X e Y, com o ícone do item registrado em cada um.
- A corrida (Running Shoes): ligada ou desligada, sem chave estilo celular.

### Comandos da batalha

- FIGHT, BAG, POKéMON, RUN.
- No FIGHT: o ícone, o nome e o nível do Pokémon que está lutando.
- Em batalha selvagem, a **Poké Ball rápida**: um botão a mais, com o ícone e a quantidade da bola e duas setas para
  trocar de bola. Desenhe as duas versões dos comandos: **com** e **sem** a Poké Ball rápida.
- No Safari Zone: BALL, BAIT, ROCK, RUN no lugar dos comandos (desenhe essa variante).
- As 6 Poké Balls do time (desmaiado, com status, vaga vazia).
- Em batalha dupla, os comandos aparecem para cada Pokémon do jogador, um de cada vez (o mesmo layout).

### Golpes da batalha

- Os 4 golpes: nome, tipo, poder, precisão e PP (vaga vazia e golpe sem PP aparecem diferentes).
- Um painel de descrição do golpe escolhido, com: efetividade contra o adversário, STAB, chance de efeito e o texto
  físico/especial (cada um pode ser ligado ou desligado nas opções).
- CANCEL, para voltar aos comandos.
- Em batalha dupla, a **escolha de alvo**: até 4 alvos com ícone e nome (desenhe essa variante).

## O que entregar

O zip de handoff do Claude Design (README + .dc.html) com cada elemento das telas acima e a posição e o tamanho
dele em pixels a 1x (x, y, largura, altura). Telas: HOME; comandos com e sem a Poké Ball rápida; Safari; golpes;
escolha de alvo.
