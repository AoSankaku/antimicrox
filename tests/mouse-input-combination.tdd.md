# Explicit controller activation: TDD evidence

## Source and user journey

The journey was derived from the request in this task:

> As an AntiMicroX user with multiple connected controllers, I want to explicitly enable or disable each controller
> so that no controller becomes active because of an implicit strength-based selection.

The Controller menu contains one checkable action per connected controller. Every mapped input from an unchecked
controller is ignored. If multiple controllers are checked, their mapped input is combined. Disabling a controller
also releases its currently active mapped output.

## RED and GREEN evidence

| Guarantee | Test or command | Type | Result | Evidence |
| --- | --- | --- | --- | --- |
| Multiple explicitly checked controllers contribute input | `multiple explicitly enabled sources are combined` | Unit | PASS | `ctest --test-dir build-tdd -R MouseInputSelectionTests --output-on-failure` |
| Only the checked controller contributes when one is enabled | `only explicitly enabled source is used` | Unit | PASS | Unchecked candidates are filtered without inspecting input strength |
| No controller input is produced when every controller is unchecked | `no enabled source produces no movement` | Unit | PASS | The selected list is empty |
| Invalid or disconnected candidates are ignored | `invalid candidates are ignored` | Unit | PASS | Null button and source candidates are filtered |
| A replayed or multiply registered button contributes once | `the same pending button contributes only once` | Unit | PASS | Selection de-duplicates identical button pointers while still combining distinct controllers |

## Validation

- RED: `cmake --build build-tdd --target MouseInputSelectionTests` failed because the old implicit-selection API
  required combination state and an active-source pointer.
- GREEN: the same target built and `ctest` reported 1/1 passing.
- Windows Qt6 application build: `cmake --build build-tdd --target antimicrox` passed.
- Japanese translation validation: `lrelease antimicrox_ja.ts` generated the QM file successfully.
- Production mouse-input filtering coverage: `gcov` reported 87.50% of lines and 100% of branches executed in
  `src/mouseinputselection.cpp`.

Checkpoint commits were intentionally not created because this repository's local instructions reserve commits for the
user.
