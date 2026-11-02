# meta-green-demo

The product layer of the reference system. It holds what a product would define: its image and the packages in it. The distribution comes from [meta-green](../meta-green/README.md), and the machine from its BSP layer.

| Item | Name | Purpose |
|---|---|---|
| Image | `green-image` | Small reference image |
| Package group | `packagegroup-green` | What the reference image adds |

**Depends on:** `meta` (openembedded-core), `meta-green`.

Replace this layer with your own product layer, and keep `meta-green`.
