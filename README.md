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

The heavy comparison uses seed 1001, 60 sensors, 30 drones, 30 supply crates, and one summary actor. Each run lasts 95 seconds; server network metrics start after 35 seconds and cover a 60-second window at one-second intervals. Three valid Generic/Iris pairs form the averages above.

Generic has role filtering disabled, while Iris has role filtering enabled. This measures the combined replication-mode and interest-policy setup. It does not isolate the effect of switching replication engines under an identical policy, and no CPU-efficiency conclusion is drawn from these bandwidth measurements.

The smaller 12-detail-actor runs met the expected filtering counts but did not show a bandwidth improvement. Early heavy runs used stale client/server binaries and were excluded from the reported heavy results.

## Implementation Flow

1. `IrisDemoGameMode::PostLogin` assigns each connection a role and a zone through `RelayPlayerState`.
2. Sensors, drones, and supply crates carry zone/category/detail metadata. Operational summary data has a separate actor.
3. `ApplyScenarioARoleBasedFiltering` obtains the Iris replication system and bridge, creates exclusion groups, and adds actor replication handles to the appropriate group.
4. Each connection receives Allow/Disallow statuses for detail-zone and summary groups based on its role. The filtering setup is also retried after a short delay to allow replication handles to initialize.
5. Client snapshots count the received actors; the server records NetDriver and per-connection outgoing metrics.

Scenario C uses `RelayCargoInventoryComponent` to create and retain cargo-item objects, while registering and unregistering them through the owner actor's registered subobject list. `RelayCargoItem` registers Iris replication fragments and replicated properties. Pointer/array replication and item-property replication are separate parts of this path.

## Current Status

- Scenario A role-based filtering: implemented and measured.
- Scenario A profiling capture path: implemented.
- Scenario C UObject/subobject replication: minimum implementation and build validation complete; the inventory component owns the collection and uses the owner actor's subobject registration API.
- Multi-item runtime verification, inventory visibility policies, prioritization, push-model stress cases, and seamless travel are outside the measured Scenario A result.

## Build

The project targets Unreal Engine 5.7.1 on Windows with a Visual Studio C++/Unreal toolchain. Set the environment variable to the Engine directory before using the scripts:

```powershell
$env:UE_571_ENGINE_DIR = "<Unreal Engine 5.7.1 installation>/Engine"
```

```bat
Tools\BuildEditor.bat
```

The expected editor target is `IrisDemoEditor Win64 Development`.

## Local Comparison Runs

Editor-based dedicated server and three clients:

```bat
Tools\RunLocalMultiplay.bat --no-iris
Tools\RunLocalMultiplay.bat --iris
```

The default map is `Content/ThirdPerson/Lvl_ThirdPerson.umap`. These commands run the Generic control and Iris role-filtered paths separately.

For the standalone Server/Client path used in the recorded measurements, use an engine build supporting these targets and prepare the binaries and cooked content:

```bat
Tools\BuildServer.bat
Tools\BuildClient.bat
Tools\CookClientServer.bat
```

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

## Measurement Outputs and Validity

Each run has a unique RunId and writes `run.json`, `client_snapshots.csv`, and `server_network_metrics.csv` under `Saved/ScenarioA/Runs/<RunId>/`. Cooked client/server runs may write to their separate `Saved/Cooked/WindowsClient` and `Saved/Cooked/WindowsServer` project folders. These generated outputs are kept locally.

A valid comparison requires matching seed/count/update/window settings, three role snapshots, and server metrics covering three connections and the full 60-second window. `run.json` records the replication mode, role-filtering setting, and effective run configuration. Client snapshots establish actor receipt; per-connection metrics record `OutBytesPerSecond` and `OutTotalBytes`. The table reports per-role bandwidth averages and NetDriver total-byte deltas for the measurement window.

The heavy Generic/Iris pairs were `ScenarioA_GenericHeavy_002` / `ScenarioA_IrisHeavy_002`, `ScenarioA_20260601_201201` / `ScenarioA_20260601_201615`, and `ScenarioA_20260601_203950` / `ScenarioA_20260601_204211`.

## Code Reading Order

- [RelayPlayerState](Source/IrisDemo/ScenarioA/RelayPlayerState.h): connection role and assigned zone
- [IrisDemoGameMode](Source/IrisDemo/IrisDemoGameMode.cpp): actor spawn, role assignment, group filters, and measurement lifecycle
- [RelayInterestTypes](Source/IrisDemo/ScenarioA/RelayInterestTypes.h): replicated actor metadata
- [IrisDemoPlayerController](Source/IrisDemo/IrisDemoPlayerController.cpp): client snapshots
- [RelayCargoInventoryComponent](Source/IrisDemo/ScenarioC/RelayCargoInventoryComponent.cpp) and [RelayCargoItem](Source/IrisDemo/ScenarioC/RelayCargoItem.cpp): Scenario C object lifecycle and subobject replication

## Limitations

- Results cover a local dedicated-server experiment with a small role/zone policy. They do not establish production-scale CPU performance or a universal Iris bandwidth advantage.
- Scenario C is disabled by default and excluded from Scenario A bandwidth comparisons.
- The inventory-component extension has build validation; multi-item runtime behavior and role/owner-only visibility integration are not part of the measured result.
- Raw runtime logs, profiling traces, and generated CSVs are not distributed with the source repository.

## Repository Notes

Generated Unreal Engine folders and profiling outputs are intentionally excluded from source control. `.uasset` and `.umap` files are configured for Git LFS tracking.

Unreal Engine and Epic-provided assets remain subject to Epic's license terms. This repository is intended as a technical portfolio project for Unreal Engine networking and replication work.
