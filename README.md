# Quick Node Layout 1.1 — Unreal Engine 5.6

Plugin de editor para organizar nós de Blueprints, incluindo o Graph dos Widget Blueprints.

## Uso

1. Abra um Blueprint e entre no gráfico desejado.
2. Clique em **Organize Nodes** na barra superior e escolha um estilo.
3. Com nós selecionados, apenas a seleção é organizada. Sem seleção, o gráfico inteiro é organizado.
4. Selecionar uma caixa de comentário inclui os nós contidos nela.
5. **Ctrl+Z** desfaz a operação inteira. O plugin não salva automaticamente.

O botão fica desabilitado no Designer, durante Play e em gráficos incompatíveis ou não editáveis.

## Estilos disponíveis

| Opção | Comportamento |
| --- | --- |
| **Human Flow** | Mantém o primeiro caminho de execução na mesma linha, separa ramificações e agrupa dependências de dados abaixo do consumidor. Comece por esta opção. |
| **Compact Flow** | Usa a mesma estratégia com espaçamentos menores. |
| **Layered Layout** | Mantém o algoritmo original, distribuindo todas as dependências em colunas. |
| **Gentle Cleanup** | Preserva a disposição existente, alinha à grade e desloca para baixo os nós sobrepostos. |

Dependências compartilhadas são posicionadas uma única vez. Loops e pontos de união podem precisar de ajustes manuais. O menu, tooltips, notificações e o nome da operação de desfazer estão em inglês.

## Organização

- Fluxo principal da esquerda para a direita, com maior peso para fios de execução.
- Dependências de dados distribuídas antes dos consumidores.
- Ordenação de ramificações para reduzir cruzamentos; espaçamento conforme o tamanho renderizado dos nós.
- Blocos desconectados separados, caixas de comentário reajustadas e posições alinhadas à grade.
- Tratamento de ciclos sem remover fios. Nós de reroute também são preservados.
- Ao organizar uma seleção, o bloco pode ser deslocado para baixo para evitar os nós externos.

Usa uma heurística local, sem IA, rede, tokens ou dependências pagas. Não garante o mínimo matemático de cruzamentos. Comentários sobrepostos e grafos com muitos ciclos podem exigir ajustes manuais. Limite de 2000 nós por operação; selecione trechos em gráficos maiores. Se o tamanho visual ainda não estiver disponível, usa uma estimativa conservadora.

Somente posições e dimensões de comentários são alteradas. Não compila Blueprints, altera conexões, remove nós ou muda valores.

## Instalação

O pacote desta versão é `QuickNodeLayout_1.1.0_UE5.6.1_Win64.zip`, para Unreal Engine **5.6.1 / Windows 64 bits / build 44394996**. A versão anterior foi preservada separadamente.

1. Feche o editor do projeto de teste.
2. Extraia o ZIP e copie a pasta **QuickNodeLayout** para `SeuProjetoDeTeste/Plugins/QuickNodeLayout`.
3. Confira que o descritor está em `Plugins/QuickNodeLayout/QuickNodeLayout.uplugin`, sem uma pasta duplicada entre eles.
4. Reabra o projeto e confira **Edit > Plugins > Quick Node Layout**.
5. Abra o Graph de um Blueprint, selecione um pequeno trecho e escolha **Organize Nodes > Human Flow**. Confira a disposição e teste **Ctrl+Z** antes de salvar.

Para atualizar, feche a Unreal antes de substituir a pasta antiga `QuickNodeLayout` pela nova. Não mantenha duas cópias do mesmo plugin nas pastas de plugins do projeto e do motor.

Não é preciso instalar nada no motor. A DLL depende das bibliotecas padrão do editor Unreal; o algoritmo não usa rede, Python, IA, MetaHuman ou um editor remoto. Outra versão/build da Unreal exige recompilar os fontes incluídos. O código-fonte também está presente no pacote para ajustes e recompilação.

## Validação

Os testes da versão 1.1 passaram nos quatro estilos: execução em linha, agrupamento de dados, redução de área no modo compacto, preservação de posições no modo suave, ciclos e ausência de sobreposições. Incluem 30 grafos pseudoaleatórios e 2000 nós por estilo; a etapa com 2000 nós nos quatro modos e verificação de sobreposições levou 130 ms neste computador.

A DLL compilou diretamente com MSVC em 24 segundos, sem executar o gerador de código da Unreal nem modificar o ArchVisualizer. **O teste visual do novo menu e dos estilos será feito pelo usuário no projeto de teste.**
