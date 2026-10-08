# Quick Node Layout 1.1 — Unreal Engine 5.6

Editor plugin for organizing Blueprint nodes, including the Widget Blueprint graph.

## Use

1. Open a Blueprint and navigate to the desired graph.
2. Click **Organize Nodes** in the top bar and choose a style.
3. If nodes are selected, only the selection is organized. If nothing is selected, the entire graph is organized.
4. Selecting a comment box includes the nodes contained within it.
5. **Ctrl+Z** undoes the entire operation. The plugin does not auto-save.

The button is disabled in the Designer, during Play mode, and for incompatible or non-editable graphs.

## Available Styles

| Option | Behavior |
| --- | --- |
| **Human Flow** | Keeps the primary execution path on the same line, separates branches, and groups data dependencies below the consumer. Start with this option. |
| **Compact Flow** | Uses the same strategy with tighter spacing. |
| **Layered Layout** | Retains the original algorithm, distributing all dependencies into columns. |
| **Gentle Cleanup** | Preserves the existing layout, aligns to the grid, and shifts overlapping nodes downward. |

Shared dependencies are positioned only once. Loops and join points may require manual adjustments. The menu, tooltips, notifications, and undo operation name are in English.

## Organization

- Primary flow from left to right, prioritizing execution wires.
- Data dependencies distributed before consumers.
- Branch ordering to reduce crossings; spacing based on rendered node sizes.
- Disconnected blocks separated, comment boxes resized, and positions grid-aligned.
- Cycle handling without removing wires; reroute nodes are preserved.
- When organizing a selection, the block may be shifted downward to avoid external nodes.

Uses a local heuristic; no AI, networking, tokens, or paid dependencies involved. Does not guarantee the mathematical minimum number of crossings. Overlapping comments and graphs with many cycles may require manual adjustments. Limit of 2,000 nodes per operation; select specific sections in larger graphs. Uses a conservative estimate if visual size data is unavailable.

Only comment positions and dimensions are modified. Does not compile Blueprints, alter connections, remove nodes, or change values.



-------------------------- PT-BR---------------------------------


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

