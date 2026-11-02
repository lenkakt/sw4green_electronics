# Designing Embedded Software for Longevity

Software often decides when hardware becomes obsolete. This text explains how embedded software design can make devices last longer, easier to repair and less wasteful. It is a result of the Technical University of Liberec's work in the European project [EECONE](https://www.eecone.com).

## 1. The problem

E-waste is any waste that contains a printed circuit board or electronic components.

- **Toxic:** it contains lead, mercury and cadmium.
- **A waste of resources:** it contains gold, silver, copper and other precious metals.
- **Difficult to recycle:** materials are combined, and some parts cannot be recycled at all.
- **Growing fast:** 62 billion kg in 2022, and 82 billion kg expected by 2030 ([Global E-waste Monitor 2024](https://ewastemonitor.info/)).
- **Poorly collected:** 42.8% is formally collected and recycled in Europe, and 11.8% in Asia.

Electronics also have a CO₂ footprint, mostly from manufacturing and then from operation, and semiconductor manufacturing uses a great deal of water.

The decisions that allow a device to be repaired, updated or reused have to be made at the beginning of the design. They are very hard to add later.

## 2. The 6R framework

| R | Goal | What software can do |
|---|---|---|
| **Reduce** | Use fewer rare, valuable and toxic materials. | Indirect: power efficiency means fewer batteries. |
| **Reliability** | Make products last longer. | A lot: modular design, fault tolerance, health monitoring, updates in the field. |
| **Repair** | Fix the faulty part, not the whole device. | A lot: a replacement part may be different hardware, so software must handle a changed configuration. |
| **Reuse** | Salvage working parts for other products. | Only if the hardware allows it. Then the same practices as for repair apply. |
| **Refurbish** | Renew a critical part or give the product a new purpose. | Only if the hardware allows it. Then new software can bring a new function. |
| **Recycle** | Recover materials at end of life. | Nothing directly: no software runs at this stage. |

Software has the strongest influence on **Reliability** and **Repair**. It helps **Reduce** through power efficiency, and supports **Reuse** and **Refurbish** where the hardware design makes them possible.

**The main goal for software is to extend the lifetime of electronics through better design, making hardware repairable, modular and updatable.**

## 3. Eight software design requirements

1. **Modularity and clear interfaces:** replaceable components with well-documented, stable APIs.
2. **Hardware abstraction:** no hard-coded board parameters in application logic. Hardware is described in configuration, such as device trees.
3. **Reproducible build environment:** versioned toolchains and dependencies, containerised and scripted builds, and a backup of every input.
4. **Long-term maintainability:** simple code, consistent style, clear ownership and documentation.
5. **Secure and robust updates:** signed images, rollback and versioning, with the update mechanism designed in from the start.
6. **Diagnostics and observability:** logging, health metrics, watchdogs and error reporting, with telemetry kept to a reasonable size.
7. **Vendor-agnostic design:** open standards and portable libraries.
8. **Power-aware implementation:** no busy loops, event-driven design, and peripherals and radios switched off when not in use.

## 4. Check your design

The [main checklist](../checklists/main_checklist.md) turns these requirements into concrete questions. The platform checklists show how to verify each question on a reference application:

- [Zephyr RTOS: greenhouse controller](../checklists/zephyr_checklist.md)
- [Yocto Project/OpenEmbedded: green distribution on qemuarm](../checklists/yocto_checklist.md)

## 5. Legislation

- [Regulation (EU) 2024/1781](https://eur-lex.europa.eu/eli/reg/2024/1781/oj/eng): Ecodesign for Sustainable Products. It replaces [Directive 2009/125/EC](https://eur-lex.europa.eu/eli/dir/2009/125/oj/eng).
- [Directive (EU) 2024/1799](https://eur-lex.europa.eu/eli/dir/2024/1799/oj): common rules promoting the repair of goods.
- [Regulation (EU) 2024/2847](https://eur-lex.europa.eu/eli/reg/2024/2847/oj/eng): Cyber Resilience Act.

## Acknowledgement

The EECONE project is supported by the Chips Joint Undertaking and its members, including the top-up funding by National Funding Authorities from involved countries under grant agreement no. 101112065. Co-funded by the European Union and the Czech Ministry of Education, Youth and Sports (project 9A23007).
