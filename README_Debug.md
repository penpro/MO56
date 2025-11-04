# MO56 Possession & Inventory Debugging

The possession and inventory changes add a structured logging pipeline that is
useful while diagnosing controller swaps, input restoration, and save/load
issues.

## Debug Log Subsystem

* **Subsystem:** `UMO56DebugLogSubsystem` (auto-instantiated as a game instance
  subsystem).
* **Log Files:** Writes to `Saved/MO56Logs/Session-<UTC timestamp>.log` by
  default. When JSON output is enabled the subsystem emits JSON Lines in the
  same directory.
* **Ring Buffer:** Maintains an in-memory queue (default 128 events) and flushes
  to disk when full or when `MO56.Debug.Flush` is executed.

### Console Variables / Commands

| Name | Description |
| --- | --- |
| `MO56.Debug.Enable` | Toggle the subsystem (0 = disabled, 1 = enabled). |
| `MO56.Debug.Json` | Switch between text (0) and JSON (1) output formats. |
| `MO56.Debug.Flush` | Immediately flush all buffered events to disk. |

Enable the subsystem through `MO56.Debug.Enable 1` (or `-MO56.Debug.Enable=1`
in command line arguments). Use `-LogJson` on the command line to default to
JSON logging for the entire session.

### Code Helpers

Use the macro `MO56_DEBUG(System, Action, Detail, PlayerId, PawnId)` from C++ to
publish events. It resolves the debug subsystem automatically and no-ops when
logging is disabled.

### Verbosity Configuration

The custom log category `LogMO56Debug` defaults to `Verbose` via
`Config/DefaultEngine.ini` so that verbose log output is captured without
additional configuration.

## Save-System Guardrails

Inventory save records now track their owning `FGuid`. During load the subsystem
asserts that the owner in the save file matches the character receiving the
inventory to prevent cross-contamination while switching pawns.
