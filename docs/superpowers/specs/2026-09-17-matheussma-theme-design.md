# MatheusSMA theme — desenho

Data: 2026-09-17
Status: aprovado para implementação

## Objetivo

Um único mod do Windhawk, de propriedade nossa, que junta o motor de estilo do
**Windows 11 Taskbar Styler** (m417z, v1.10) com o comportamento de animação do
**Taskbar Dock Animation** (Ph0en1x-dev, v1.9.2), servindo de base para o que for
adicionado depois.

O mod se chama **MatheusSMA theme**, `@id: matheussma-theme`.

## Restrições

**Um mod é um arquivo.** O wiki do Windhawk é explícito: "Windhawk mods are
developed as single C++ files that are compiled into dynamic libraries." Não há
suporte a múltiplos arquivos nem a `#include` local — confirmado nos dois fontes
originais, nenhum deles inclui arquivo próprio. O mod fundido é, portanto, um
`.wh.cpp` só.

**Licença: GPLv3.** O styler declara GPLv3 no cabeçalho. O dock-animation não
declara licença, e o README do `ramensoftware/windhawk-mods` estabelece que "Mods
which don't specify a license are submitted under the MIT license" — logo, MIT.
MIT é compatível com GPLv3, e a obra derivada sai GPLv3, preservando o aviso de
copyright MIT. O repositório é público, então os créditos a m417z e Ph0en1x-dev
são exigência legal, não cortesia.

**Sem instalação por linha de comando.** O `command-line.txt` do Windhawk expõe
apenas `-tray-only`, `-exit`, `-restart` e `-safe-mode`. A instalação e a
recompilação acontecem pela interface do Windhawk.

## Escopo do que entra no arquivo

| Parte | Origem | Linhas aprox. |
|---|---|---|
| Motor de estilo (TAP de XAML Diagnostics) | styler, 10846–20769 | 9.920 |
| Tema `FrostyGlass` | styler, 8552–8811 | 260 |
| Animação completa | dock-animation, arquivo inteiro | 1.487 |
| **Total** | | **~11.700** |

Ficam de fora os outros 48 temas do styler, cerca de 10.000 linhas que nunca são
carregadas — as configurações atuais mostram `theme = FrostyGlass` e todos os
campos de estilo customizado vazios. É também a parte que mais muda no upstream,
então descartá-la reduz o atrito de sincronização pela metade.

Para trazer outro tema depois, basta copiar o bloco correspondente de
`upstream/windows-11-taskbar-styler@1.10.wh.cpp`.

## Arquitetura

### Separação

Cada motor vive no seu próprio `namespace` — `styler::` e `dock::`. Os dois
originais declaram globais com prefixo `g_` e abrem `using namespace winrt::...`
no escopo de arquivo; juntos sem namespace, colidem.

### A fronteira é o `RenderTransform`

A animação é dona do `RenderTransform` de todo elemento que anima, e assume uma
estrutura rígida: um `TransformGroup` com exatamente quatro filhos, nesta ordem —
`[0] waveScale`, `[1] waveTranslate`, `[2] bounceScale`, `[3] bounceTranslate`.
Quando não encontra isso, desiste sem avisar:

```cpp
if (!tg || tg.Children().Size() < 4) return;
```

O motor de estilo também escreve `RenderTransform`. No `FrostyGlass` isso acontece
em `TextBlock#TimeInnerTextBlock` e `TextBlock#DateInnerTextBlock` — elementos do
system tray, não os `Taskbar.TaskListButton` que a animação escala. Hoje não há
colisão por acidente de alvo, não por desenho.

**Regra:** o motor de estilo não aplica `RenderTransform` a elemento pertencente à
animação. A aplicação de estilo ganha um guard que pula essa propriedade nesses
elementos e registra um aviso no log. A falha passa a ser alta e diagnosticável em
vez de um ícone que silenciosamente para de animar.

Elemento "pertencente à animação" é o que está registrado no contexto da animação
(`DockAnimationContext::icons`), decidido por `ShouldAnimateElement`.

### Re-medição por evento, não por varredura

Hoje a animação descobre que o layout mudou varrendo o host a cada frame. Dentro
de `OnCompositionTargetRendering`, linha 1151 do original:

```cpp
HostSignature sig = ComputeHostSignature(host);
if (SigDifferent(sig, ctx.lastSig)) { RefreshIconPositions(ctx); ... }
```

`ComputeHostSignature` percorre os filhos do host acumulando um hash. Isso roda a
cada frame enquanto a animação está ativa, apenas para detectar que o estilo mexeu
na geometria.

No mod fundido, o TAP de estilo sabe exatamente quando alterou a árvore visual —
é ele quem chama `ApplyCustomizations`. Essa chamada passa a marcar um sinalizador
`g_geometryDirty`, e a animação re-mede na frame seguinte.

Ganhos: some o hash por frame, e a re-medição passa a ser correta por construção
em vez de heurística sobre um hash que pode não capturar toda mudança relevante.

A varredura por assinatura é mantida como rede de segurança para mudanças de
geometria vindas de fora do mod (mudança de DPI, rotação, o próprio Windows
remontando a barra), mas passa a rodar com intervalo, não a cada frame.

### Pontos de entrada

Um de cada, chamando os dois subsistemas em ordem definida.

| Função | Ordem | Motivo |
|---|---|---|
| `Wh_ModInit` | symbol hooks da animação, depois registro do TAP | os hooks precisam estar instalados antes de as funções de `taskbar.view.dll` executarem |
| `Wh_ModAfterInit` | animação, depois estilo | — |
| `Wh_ModSettingsChanged` | recarrega os dois | — |
| `Wh_ModBeforeUninit` | solta o TAP, depois desfaz hooks e chama `ResetAllIconScales` | sem o reset, os ícones ficam congelados em escala na barra |

### Opções de compilação

União das duas listas:

```
-lcomctl32 -lgdi32 -lole32 -loleaut32 -lruntimeobject -lshcore -lshlwapi -lwindowsapp -luser32
```

## Configurações

As chaves dos dois originais são mantidas com o nome que já têm. O styler usa
camelCase e a animação usa PascalCase; a mistura é feia, mas **não há colisão de
chave**, e renomear custaria atrito em todo port de correção vinda do upstream.
O agrupamento visual é feito pelo texto de `$name` ("Dock: ...", "Estilo: ...").

Os valores padrão são os que já estão em uso hoje, para que o mod funcione
corretamente assim que instalado:

| Chave | Padrão aqui | Padrão upstream |
|---|---|---|
| `theme` | `FrostyGlass` | `""` |
| `MaxScale` | `180` | `130` |

As demais chaves já coincidem com o padrão de fábrica.

`theme` permanece como lista suspensa, com duas opções: `None` e `FrostyGlass`.
Uma lista de uma opção seria inútil, mas `None` permite desligar o estilo sem
desinstalar o mod, que é o primeiro passo ao investigar conflito entre os motores.

`controlStyles`, `styleConstants` e `themeResourceVariables` continuam expostos —
são o caminho de extensão sem recompilar. Ajuste visual futuro entra por regra de
estilo; o C++ fica reservado para comportamento.

`xamlDiagnosticsHandling` continua exposto. Só pode existir um consumidor de XAML
diagnostics por processo: se o styler original permanecer instalado ao lado deste
mod, os dois disputam o mesmo slot.

## Repositório

```
matheussma-theme/
├─ LICENSE                                    GPLv3
├─ NOTICE.md                                  créditos e licenças de origem
├─ README.md                                  o que é, o que diverge do upstream
├─ mods/matheussma-theme.wh.cpp               o mod
├─ upstream/
│  ├─ windows-11-taskbar-styler@1.10.wh.cpp   cópia intacta
│  └─ taskbar-dock-animation@1.9.2.wh.cpp     cópia intacta
└─ scripts/
   ├─ sync.ps1                                ModsSource → repo
   └─ check-upstream.ps1                      busca versão nova e diffa contra a cópia fixada
```

`upstream/` é o que torna a atualização tratável. Quando sair a 1.11 do styler,
o diff entre a cópia fixada e a nova mostra o que mudou no motor, sem o ruído dos
temas que não carregamos.

### Direção da sincronização

**Windhawk → repositório.** O Windhawk é dono do arquivo em `ModsSource`; o
repositório é o espelho versionado.

- Na criação, uma vez: colar o fonte no "Create new mod" do Windhawk e compilar.
- Depois: editar pelo editor do Windhawk, e `sync.ps1` traz o arquivo para o
  repositório.

Essa direção foi escolhida por não depender de suposição: não está verificado se
o Windhawk reage a escrita externa em `ModsSource`. Isso será testado durante a
implementação; se funcionar, `sync.ps1` ganha a direção inversa e a edição passa
a acontecer no repositório.

## Verificação

Não há framework de teste aplicável a um mod do Windhawk. A verificação é uma
lista executável:

1. **Compila** no Windhawk. Primeiro portão real: dois fontes, ~11.700 linhas,
   precisam linkar com a união das opções de compilação.
2. **Log mostra os dois subsistemas subindo** — TAP de estilo registrado e symbol
   hooks de `taskbar.view.dll` instalados.
3. **Hover escala os ícones e o `FrostyGlass` continua aplicado.**
4. **O guard dispara.** Adicionar um `controlStyles` com `RenderTransform` em um
   `Taskbar.TaskListButton`. Deve aparecer aviso no log **e** o ícone deve
   continuar animando. Animar sem logar significa que o guard não está ativo;
   parar de animar significa que a regressão silenciosa voltou.
5. **Desinstalar devolve os ícones ao tamanho normal**, provando que
   `ResetAllIconScales` roda em `Wh_ModBeforeUninit`.

O item 4 é o teste que importa: é o único que exercita a regressão que o desenho
inteiro existe para evitar.

## Pré-condição de instalação

Os dois mods originais precisam ser desinstalados antes de habilitar este. Com os
três ativos, os symbol hooks duplicam e os dois motores de estilo disputam o slot
único de XAML diagnostics.

## Riscos conhecidos

**Clipping em `MaxScale = 180`.** O README do dock-animation recomenda manter a
escala em no máximo 130 e lista "Icons are sometimes clipped by the taskbar" como
problema aberto. A configuração atual está em 180. Isso não é regressão
introduzida pela fusão — é um bug herdado, e o primeiro candidato natural a
correção agora que o código é nosso. Fora do escopo desta entrega.

**Divergência do upstream.** A partir daqui, correções do m417z para novas builds
do Windows não chegam automaticamente. É o custo aceito de ser dono do código; a
pasta `upstream/` e o `check-upstream.ps1` existem para reduzi-lo.

**Ordem de aplicação em elementos compartilhados.** O guard cobre
`RenderTransform`. Outras propriedades de geometria (`Width`, `Height`, `Margin`,
`Padding`) são escritas pelo estilo e lidas pela animação — essa é a relação que o
`g_geometryDirty` existe para manter correta. Se surgir tremor ou posição errada
após mudança de tema, é o primeiro lugar a investigar.
