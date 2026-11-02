# Main green checklist

Answer each question with Yes or No and add a short comment. Use it at three stages: concept, prototype and final evaluation. A "No" is not a failure. It shows you a decision you can now make consciously.

The platform checklists use the same numbers: [Zephyr](zephyr_checklist.md) and [Yocto Project/OpenEmbedded](yocto_checklist.md).

| No. | Question | Yes/No, comments |
|---|---|---|
| **1** | **Reduce** | |
| 1.1 | Does the software consistently use low-power sleep modes whenever possible? | |
| 1.2 | Are busy loops avoided (such as event-driven design instead of infinite active loops)? | |
| 1.3 | Are peripherals and radios disabled when not in use? | |
| **2** | **Reliability** | |
| 2.1 | Is the software modular? | |
| 2.2 | Is the API designed to keep at least ABI stable? | |
| 2.3 | Are diagnostics implemented (logging, health monitoring, watchdogs)? | |
| 2.4 | Is the build reproducible in the future (versioned dependencies, documented build process)? | |
| 2.5 | Does the architecture avoid vendor lock-in (use of open standards and portable tools)? | |
| 2.6 | Are secure updates supported (signed updates, vulnerability patching, rollback/versioning)? | |
| **3** | **Reuse** | |
| 3.1 | Can software modules be reused in other devices or projects? | |
| 3.2 | Are interfaces clearly documented to support reuse? | |
| **4** | **Repair** | |
| 4.1 | Is hardware abstraction used? | |
| 4.2 | Can hardware components be replaced without modifying application code? | |
| 4.3 | Is the hardware configuration separated from application logic? | |
| 4.4 | Can the software be easily reconfigured and rebuilt when hardware is repaired or replaced? | |
| 4.5 | Is the system capable of updates and upgrades in the field? | |
