# IrisDemo

UE 5.7.1 Iris replication demo focused on role-based network interest filtering, reproducible Generic/Iris comparison, and bandwidth measurement.

## Portfolio Focus

`IrisDemo` is a network replication portfolio project. The goal is not to show a large synthetic benchmark, but to solve a practical multiplayer problem:

> Different player roles in the same operation should receive different network data.

The first completed scenario, **Scenario A: Role-Based Interest Filtering**, compares the legacy Generic replication path and Iris under controlled conditions.

## Scenario A Summary

Scenario A models three connection roles:

| Role | Expected network data |
|------|-----------------------|
| Commander | Full operational detail and summary |
| FieldAgent | Detail actors only for the assigned zone |
| Spectator | Summary only, no real-time detail actors |

The implementation treats role as player/connection state, not actor state:

- `ARelayPlayerState` stores the replicated operator role and assigned zone.
- Test actors expose metadata such as `ZoneId`, category, and detail/summary level.
- Iris group filtering maps actor metadata to connection-specific visibility.
- Generic replication is used as a control run with filtering disabled.

## Measured Result

In the 120 detail-actor heavy run, Generic replication sends all detail actors to all roles. Iris filtering keeps the Commander view complete while reducing FieldAgent and Spectator detail replication.

| Mode | Commander | FieldAgent | Spectator |
|------|-----------|------------|-----------|
| Generic | 120 detail actors | 120 detail actors | 120 detail actors |
| Iris filtering | 120 detail actors | 40 detail actors | 0 detail actors |

Across three heavy runs, the average outgoing bandwidth changed in the same direction:

| Role / Scope | Generic average | Iris average | Difference |
|--------------|-----------------|--------------|------------|
| Commander AvgOutBytesPerSecond | 2317.69 | 2100.40 | -9.4% |
| FieldAgent AvgOutBytesPerSecond | 2326.39 | 1752.34 | -24.7% |
| Spectator AvgOutBytesPerSecond | 2311.83 | 1576.34 | -31.8% |
| NetDriver DeltaOutTotalBytes | 416845.00 | 326283.00 | -21.7% |

See [Docs/PerformanceRuns.md](Docs/PerformanceRuns.md) for the full measurement notes and run validity criteria.

## Current Status

- Scenario A role-based filtering: implemented and measured.
- Scenario A profiling capture path: documented with trace run notes.
- Scenario C UObject/subobject replication: minimum implementation and build validation complete, expanding toward inventory policy integration.
- Prioritization, push model stress cases, and seamless travel verification remain roadmap items.

## Build

The project targets Unreal Engine 5.7.1.

```bat
Tools\BuildEditor.bat
```

The expected editor target is `IrisDemoEditor Win64 Development`.

## Local Comparison Runs

Generic control run:

```bat
Tools\RunLocalBinariesMultiplay.bat --no-iris
```

Iris filtering run:

```bat
Tools\RunLocalBinariesMultiplay.bat --iris
```

Scenario C can be enabled separately so it does not affect Scenario A measurements:

```text
-ScenarioCEnableCargoSubobjects=1
```

## Key Documents

- [Docs/planning.md](Docs/planning.md): full Iris Relay Lab planning.
- [Docs/ScenarioA_RoleBasedFiltering.md](Docs/ScenarioA_RoleBasedFiltering.md): Scenario A design, checklist, and results.
- [Docs/PerformanceRuns.md](Docs/PerformanceRuns.md): Generic/Iris actor count and bandwidth measurements.
- [Docs/DebuggingNotes.md](Docs/DebuggingNotes.md): invalid run analysis and debugging notes.
- [Docs/ScenarioC_UObjectSubobjectReplication.md](Docs/ScenarioC_UObjectSubobjectReplication.md): UObject/subobject replication plan and current state.
- [Docs/Portfolio.md](Docs/Portfolio.md): short portfolio summary.

## Repository Notes

Generated Unreal Engine folders and profiling outputs are intentionally excluded from source control. `.uasset` and `.umap` files are configured for Git LFS tracking.

Unreal Engine and Epic-provided assets remain subject to Epic's license terms. This repository is intended as a technical portfolio project for Unreal Engine networking and replication work.
