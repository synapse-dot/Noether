# Noether Language Specification (v1.0)

Noether is a domain-specific language engineered for physical modelling and simulation. It operates on the principle of dual description, where the user provides both a physical and a mathematical representation of a system, allowing the internal engine to reconcile and execute the resulting physics.

## Core Philosophy

The fundamental objective of Noether is to allow users to describe the physical world while the language handles the underlying physics and subsequent visualisation. 

* **Integrated Physics:** Essential physical laws are native to the environment.
* **The Reconciler:** A strict validation engine that ensures the physical and mathematical descriptions are mutually consistent. Contradictions or ambiguities result in hard compile-time errors.
* **Automated Derivation:** The language can automatically derive equations of motion when a Lagrangian is provided.

---

## Project Specifications

* **Language Name:** Noether
* **CLI Binary:** `dash`
* **File Extension:** `.noe`
* **Execution Model:** Interpreted (Compiler pending)

---

## Type System and Dimensions

Noether employs a rigorous, SI-aligned dimensional type system. Dimensional consistency is enforced at compile-time.

### Base Dimensions
| Symbol | Physical Dimension |
| :--- | :--- |
| **L** | Length |
| **M** | Mass |
| **T** | Time |
| **I** | Electric Current |
| **K** | Thermodynamic Temperature |
| **N** | Amount of Substance |
| **J** | Luminous Intensity |

### Implementation Details
* **Dimensionless Types:** `int` and `real` types are distinct and are automatically lifted when combined with dimensioned types.
* **Derived Dimensions:** Composed using multiplication (`*`) and exponentiation (`^`). For example, Force is represented as `[M*L*T^-2]`.
* **Normalisation:** Numeric literals are auto-normalised to scientific notation internally.
* **Syntax:** Dimensions may be applied as a suffix or a prefix to an identifier.

---

## Syntax Conventions

* **Structural Blocks:** Defined using braces `{}`.
* **Comments:** Prefixed with the `#` symbol.
* **Naming Conventions:**
    * Variables must be lowercase (enforced).
    * Dimension symbols must be uppercase (enforced).
* **Mathematical Notation:** Prioritises ASCII-first notation with LaTeX-inspired fallbacks (e.g., `\partial`, `\integral`) for complex operations.

---

## Execution Pipeline

The `dash` binary processes `.noe` files through the following stages:

1.  **Lexical and Syntactic Analysis:** Tokenisation and generation of the Abstract Syntax Tree (AST).
2.  **Semantic Analyser:** Validation of variable conventions and logic.
3.  **The Reconciler:** The critical stage where physical and mathematical layers are checked for consistency. It fills in gaps in the physical description and derives necessary equations.
4.  **Intermediate Representation (IR):** Generation of the code for the backend.
5.  **Backend Execution:** Processing via the integrator and outputting to the renderer.

---

## Backend and Output

* **Integrators:** Uses RK4 (Runge-Kutta 4th Order) by default; Euler integration is available as an alternative.
* **Renderer:** Utilises OpenGL for real-time simulation windows.
* **Data Export:** Supports optional data file exportation for external analysis.

For v0.1, the language remains interpreted, focusing on real-time simulation accuracy and strict dimensional verification.


## v1.0 Implemented Components

- The lexer, parser, and semantic analyser pipeline is executable through the `dash` CLI.
- CLI usage: `./dash <program.noe>`
- Includes a runnable sample program: `example_v1.noe`.
