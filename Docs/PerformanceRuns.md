# Scenario A Performance Runs

이 문서는 Scenario A의 actor count와 outgoing bandwidth 측정 결과를 같은 RunId 기준으로 정리하기 위한 문서다.

핵심 목적은 “Iris filtering으로 수신 actor 수가 줄었다”에서 멈추지 않고, 같은 조건에서 서버 outgoing bandwidth도 함께 달라졌는지 확인하는 것이다.

## 1. 데이터 준비 상태

2026-06-01 기준 12 actor baseline, 120 actor heavy 1차 비교, 120 actor heavy 반복 측정 2쌍을 확보했다.

현재 상태:

- Iris filtering run: client snapshot CSV와 server bandwidth CSV 확보됨
- Generic baseline run: client snapshot CSV와 server bandwidth CSV 확보됨
- `ScenarioA_GenericHeavy_001` / `ScenarioA_IrisHeavy_001`: 실행은 완료됐지만 오래된 Server/Client 바이너리로 실행되어 heavy 측정으로는 무효
- `ScenarioA_GenericHeavy_002` / `ScenarioA_IrisHeavy_002`: 새 Server/Client 바이너리로 재측정했고 120 actor, 60초 metrics window 조건을 만족한다.
- `ScenarioA_20260601_201201` / `ScenarioA_20260601_201615`: 같은 heavy 조건으로 반복 측정했고 client/server 산출물과 60초 metrics window 조건을 만족한다.
- `ScenarioA_20260601_203950` / `ScenarioA_20260601_204211`: 세 번째 heavy 반복 측정이다. no-iris run에서 서버가 남은 client 하나보다 먼저 종료된 정황은 있었지만, client snapshot 3개와 `ConnectionCount=3` 기준 60초 metrics window 61 samples가 모두 있어 측정값은 유효로 판정한다.
- 결론: 12 actor run에서는 actor count 감소만 확인되고 bandwidth 개선은 확인되지 않았다. 120 actor heavy run에서는 3회 모두 Iris filtering에서 FieldAgent/Spectator의 actor count와 outgoing bandwidth가 함께 감소했다.

필수 비교 run:

| Run type | 필수 상태 | 현재 상태 | 비고 |
|----------|-----------|-----------|------|
| Generic baseline | 필요 | 확보 | `-UseIrisReplication=0`, `ScenarioAEnableRoleFiltering=0`, RunId `ScenarioA_GenericBaseline_001` |
| Iris filtering on | 필요 | 확보 | `-UseIrisReplication=1`, `ScenarioAEnableRoleFiltering=1`, RunId `ScenarioA_IrisFiltering_001` |
| Generic heavy | 필요 | 확보 | RunId `ScenarioA_GenericHeavy_002`, detail actor 120개 조건 실행 |
| Iris heavy | 필요 | 확보 | RunId `ScenarioA_IrisHeavy_002`, detail actor 120개 조건 실행 |
| Generic heavy repeat | 필요 | 확보 | RunId `ScenarioA_20260601_201201`, detail actor 120개 조건 실행 |
| Iris heavy repeat | 필요 | 확보 | RunId `ScenarioA_20260601_201615`, detail actor 120개 조건 실행 |
| Generic heavy final repeat | 필요 | 확보 | RunId `ScenarioA_20260601_203950`, detail actor 120개 조건 실행 |
| Iris heavy final repeat | 필요 | 확보 | RunId `ScenarioA_20260601_204211`, detail actor 120개 조건 실행 |

## 2. 유효한 run 판정 기준

하나의 RunId가 성능 비교에 쓰이려면 아래 파일이 모두 있어야 한다.

```text
Saved/ScenarioA/Runs/<RunId>/run.json
Saved/ScenarioA/Runs/<RunId>/client_snapshots.csv
Saved/ScenarioA/Runs/<RunId>/server_network_metrics.csv
```

패키징된 client/server 실행에서는 저장 위치가 분리될 수 있다. `ScenarioA_GenericBaseline_001`은 아래 위치에서 확인했다.

```text
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericBaseline_001/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericBaseline_001/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericBaseline_001/server_network_metrics.csv
```

`ScenarioA_IrisFiltering_001`은 아래 위치에서 확인했다.

```text
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisFiltering_001/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisFiltering_001/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisFiltering_001/server_network_metrics.csv
```

`ScenarioA_GenericHeavy_001`과 `ScenarioA_IrisHeavy_001`은 server 산출물만 확인했다. client snapshot CSV는 발견되지 않았다.

```text
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_001/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_001/server_network_metrics.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_001/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_001/server_network_metrics.csv
```

`ScenarioA_GenericHeavy_002`와 `ScenarioA_IrisHeavy_002`는 아래 위치에서 client/server 산출물을 모두 확인했다.

```text
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_002/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_002/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_002/server_network_metrics.csv
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_002/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_002/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_002/server_network_metrics.csv
```

`ScenarioA_20260601_201201`와 `ScenarioA_20260601_201615`도 아래 위치에서 client/server 산출물을 모두 확인했다.

```text
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_201201/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_201201/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_201201/server_network_metrics.csv
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_201615/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_201615/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_201615/server_network_metrics.csv
```

`ScenarioA_20260601_203950`와 `ScenarioA_20260601_204211`도 아래 위치에서 client/server 산출물을 모두 확인했다.

```text
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_203950/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_203950/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_203950/server_network_metrics.csv
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_204211/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_204211/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_20260601_204211/server_network_metrics.csv
```

`run.json`에서 확인할 것:

- `mode`
- `roleFiltering`
- `seed`
- `sensorCount`
- `droneCount`
- `supplyCrateCount`
- `runDuration`
- `networkMetricsInterval`
- `networkMetricsStartDelay`
- `networkMetricsDuration`

`client_snapshots.csv`에서 확인할 것:

- Commander row 존재
- FieldAgent row 존재
- Spectator row 존재
- 각 role의 `DetailActorTotal`, `SummaryCount`

`server_network_metrics.csv`에서 확인할 것:

- `Scope=NetDriver` row 존재
- `Scope=Connection` row 존재
- 각 connection의 `Role`, `Zone`, `OutBytesPerSecond`, `OutTotalBytes`

## 3. Generic baseline 확인 결과

다음 조건을 만족하는 run이 Generic baseline으로 유효하다.

```text
run.json:
  mode == "Generic"
  roleFiltering == false

client_snapshots.csv:
  Commander / FieldAgent / Spectator 모두 DetailActorTotal=12

server_network_metrics.csv:
  Scope=Connection row가 role별로 존재
```

`ScenarioA_GenericBaseline_001`은 위 조건을 만족한다.

실행 조건:

| 항목 | 값 |
|------|----|
| RunId | `ScenarioA_GenericBaseline_001` |
| Timestamp | `2026-05-30T15:57:56.733Z` |
| Map | `Lvl_ThirdPerson` |
| ServerType | `DedicatedServer` |
| Mode | `Generic` |
| Iris override | `Generic` |
| RoleFiltering | `false` |
| Seed | `1001` |
| Detail actors | Sensor 6, Drone 3, SupplyCrate 3, total 12 |
| Summary actors | 1 |
| Configured runDuration | 30.000 |
| Network metrics interval | 1.000 |

Client snapshot:

| Role | Zone | SensorCount | DroneCount | SupplyCrateCount | DetailActorTotal | SummaryCount | DetailMaxSequence |
|------|------|-------------|------------|------------------|------------------|--------------|-------------------|
| Commander | -1 | 6 | 3 | 3 | 12 | 1 | 22 |
| FieldAgent | 0 | 6 | 3 | 3 | 12 | 1 | 22 |
| Spectator | -1 | 6 | 3 | 3 | 12 | 1 | 22 |

## 4. Iris filtering 확인 결과

다음 조건을 만족하는 run이 Iris filtering 비교 대상으로 유효하다.

```text
run.json:
  mode == "Iris"
  roleFiltering == true

client_snapshots.csv:
  Commander DetailActorTotal=12
  FieldAgent DetailActorTotal=4
  Spectator DetailActorTotal=0

server_network_metrics.csv:
  Scope=Connection row가 role별로 존재
```

`ScenarioA_IrisFiltering_001`은 위 조건을 만족한다.

실행 조건:

| 항목 | 값 |
|------|----|
| RunId | `ScenarioA_IrisFiltering_001` |
| Timestamp | `2026-05-30T16:25:15.750Z` |
| Map | `Lvl_ThirdPerson` |
| ServerType | `DedicatedServer` |
| Mode | `Iris` |
| Iris override | `Iris` |
| RoleFiltering | `true` |
| Seed | `1001` |
| Detail actors | Sensor 6, Drone 3, SupplyCrate 3, total 12 |
| Summary actors | 1 |
| Configured runDuration | 30.000 |
| Network metrics interval | 1.000 |

Client snapshot:

| Role | Zone | SensorCount | DroneCount | SupplyCrateCount | DetailActorTotal | SummaryCount | DetailMaxSequence |
|------|------|-------------|------------|------------------|------------------|--------------|-------------------|
| Commander | -1 | 6 | 3 | 3 | 12 | 1 | 21 |
| FieldAgent | 0 | 2 | 1 | 1 | 4 | 0 | 21 |
| Spectator | -1 | 0 | 0 | 0 | 0 | 1 | 0 |

## 5. Bandwidth 집계 기준

`server_network_metrics.csv`는 configured `runDuration=30`보다 긴 시간 동안 기록될 수 있다. 현재 표는 세 connection이 모두 유지되고 role 식별이 끝난 안정 구간만 사용한다.

집계 조건:

```text
Scope == Connection
ConnectionCount == 3
Role != Unknown
Role != Unassigned
```

집계 구간:

| RunId | Window | Samples per role | NetDriver DeltaOutTotalBytes |
|-------|--------|------------------|------------------------------|
| ScenarioA_GenericBaseline_001 | `2026-05-30T15:58:07.806Z` ~ `2026-05-30T16:00:06.794Z` | 120 | 523278 |
| ScenarioA_IrisFiltering_001 | `2026-05-30T16:25:25.818Z` ~ `2026-05-30T16:26:09.813Z` | 45 | 211875 |
| ScenarioA_GenericHeavy_002 | `2026-05-30T18:22:54.087Z` ~ `2026-05-30T18:23:54.062Z` | 61 | 411758 |
| ScenarioA_IrisHeavy_002 | `2026-05-30T18:26:07.435Z` ~ `2026-05-30T18:27:07.426Z` | 61 | 325034 |
| ScenarioA_20260601_201201 | `2026-06-01T20:12:38.478Z` ~ `2026-06-01T20:13:38.459Z` | 61 | 419329 |
| ScenarioA_20260601_201615 | `2026-06-01T20:16:52.954Z` ~ `2026-06-01T20:17:52.930Z` | 61 | 327108 |
| ScenarioA_20260601_203950 | `2026-06-01T20:40:27.432Z` ~ `2026-06-01T20:41:27.426Z` | 61 | 419448 |
| ScenarioA_20260601_204211 | `2026-06-01T20:42:47.566Z` ~ `2026-06-01T20:43:47.547Z` | 61 | 326707 |

`OutBytesPerSecond`는 이 안정 구간의 단순 평균이다. `DeltaOutTotalBytes`는 같은 구간에서 마지막 `OutTotalBytes`와 첫 `OutTotalBytes`의 차이다.

NetDriver 전체 row는 `OutBytesPerSecond`가 0으로 기록되므로 현재 해석에는 connection별 값을 우선 사용한다. 12 actor run은 안정 구간 길이가 다르므로 `DeltaOutTotalBytes`끼리는 직접 비교하지 않고, role별 `AvgOutBytesPerSecond`를 우선 비교한다. 120 actor heavy run은 `MetricsWindowElapsedSeconds` 기준으로 약 60초 동일 window가 확보됐으므로 role별 평균과 `DeltaOutTotalBytes`를 함께 비교한다.

## 6. Heavy 측정 시도 판정

2026-05-30에 아래 명령을 실행했다.

```bat
Tools\RunLocalBinariesMultiplay.bat --no-iris --run-id ScenarioA_GenericHeavy_001
Tools\RunLocalBinariesMultiplay.bat --iris --run-id ScenarioA_IrisHeavy_001
```

그러나 `run.json` 기준으로 두 run 모두 heavy 조건이 아니었다.

| RunId | Mode | RoleFiltering | DetailActorTotal | RunDuration | CSV schema | Client snapshot | 판정 |
|-------|------|---------------|------------------|-------------|------------|-----------------|------|
| ScenarioA_GenericHeavy_001 | Generic | false | 12 | 30.000 | old, no `MetricsWindowElapsedSeconds` | 없음 | 무효 |
| ScenarioA_IrisHeavy_001 | Iris | true | 12 | 30.000 | old, no `MetricsWindowElapsedSeconds` | 없음 | 무효 |

원인은 `Tools\RunLocalBinariesMultiplay.bat`가 `Binaries\Win64\IrisDemoServer.exe`와 `IrisDemoClient.exe`를 실행하는데, 해당 바이너리가 새 측정 기본값 커밋 전 빌드였기 때문이다.

확인 당시 바이너리 timestamp:

| Binary | LastWriteTime |
|--------|---------------|
| `Binaries\Win64\IrisDemoServer.exe` | 2026-05-28 21:01:26 |
| `Binaries\Win64\IrisDemoClient.exe` | 2026-05-28 21:00:44 |

무효 run의 server metrics 참고값:

| RunId | Mode | Role | AvgOutBytesPerSecond | DeltaOutTotalBytes | Samples | Status |
|-------|------|------|----------------------|--------------------|---------|--------|
| ScenarioA_GenericHeavy_001 | Generic | Commander | 1482.05 | 432150 | 296 | Invalid stale-binary run |
| ScenarioA_GenericHeavy_001 | Generic | FieldAgent | 1475.36 | 431612 | 296 | Invalid stale-binary run |
| ScenarioA_GenericHeavy_001 | Generic | Spectator | 1475.61 | 435273 | 296 | Invalid stale-binary run |
| ScenarioA_IrisHeavy_001 | Iris | Commander | 1642.88 | 221946 | 136 | Invalid stale-binary run |
| ScenarioA_IrisHeavy_001 | Iris | FieldAgent | 1582.60 | 213945 | 136 | Invalid stale-binary run |
| ScenarioA_IrisHeavy_001 | Iris | Spectator | 1566.49 | 212164 | 136 | Invalid stale-binary run |

위 값은 기존 12 actor 조건에서 나온 참고값일 뿐, 120 actor 동일 window 비교에는 사용하지 않는다.

무효 판정 후 `Tools\BuildServer.bat`와 `Tools\BuildClient.bat`를 실행해 새 Server/Client 바이너리 빌드는 성공했다. 이후 같은 RunId를 재사용하지 않고 `_002` RunId로 재측정했다.

## 7. Heavy 재측정 확인

120 actor 동일 window 측정은 새로 빌드된 바이너리로 아래 명령을 실행했다.

```bat
Tools\RunLocalBinariesMultiplay.bat --no-iris --run-id ScenarioA_GenericHeavy_002
```

Iris filtering:

```bat
Tools\RunLocalBinariesMultiplay.bat --iris --run-id ScenarioA_IrisHeavy_002
```

두 run 모두 완료 후 아래 산출물을 확인했다.

```text
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_002/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_002/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_002/server_network_metrics.csv
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_002/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_002/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_002/server_network_metrics.csv
```

두 run 모두 `run.json` 기준으로 `detailActorTotal=120`, `runDuration=95.000`, `networkMetricsStartDelay=35.000`, `networkMetricsDuration=60.000`을 만족한다. `server_network_metrics.csv`에는 `MetricsWindowElapsedSeconds` 컬럼이 있다.

실행 조건:

| RunId | Mode | RoleFiltering | Timestamp | Detail actors | Summary actors | RunDuration | Metrics start/duration |
|-------|------|---------------|-----------|---------------|----------------|-------------|------------------------|
| ScenarioA_GenericHeavy_002 | Generic | false | `2026-05-30T18:22:19.000Z` | Sensor 60, Drone 30, SupplyCrate 30, total 120 | 1 | 95.000 | 35.000 / 60.000 |
| ScenarioA_IrisHeavy_002 | Iris | true | `2026-05-30T18:25:32.356Z` | Sensor 60, Drone 30, SupplyCrate 30, total 120 | 1 | 95.000 | 35.000 / 60.000 |

Client snapshot:

| RunId | Role | Zone | SensorCount | DroneCount | SupplyCrateCount | DetailActorTotal | SummaryCount | DetailMaxSequence |
|-------|------|------|-------------|------------|------------------|------------------|--------------|-------------------|
| ScenarioA_GenericHeavy_002 | Commander | -1 | 60 | 30 | 30 | 120 | 1 | 53 |
| ScenarioA_GenericHeavy_002 | FieldAgent | 0 | 60 | 30 | 30 | 120 | 1 | 53 |
| ScenarioA_GenericHeavy_002 | Spectator | -1 | 60 | 30 | 30 | 120 | 1 | 54 |
| ScenarioA_IrisHeavy_002 | Commander | -1 | 60 | 30 | 30 | 120 | 1 | 53 |
| ScenarioA_IrisHeavy_002 | FieldAgent | 0 | 20 | 10 | 10 | 40 | 0 | 54 |
| ScenarioA_IrisHeavy_002 | Spectator | -1 | 0 | 0 | 0 | 0 | 1 | 0 |

## 8. Heavy 반복 측정 확인

2026-06-01에 기본 자동 RunId로 같은 heavy 조건을 두 번 더 측정했다.

Generic replication:

```bat
Tools\RunLocalBinariesMultiplay.bat --no-iris
```

Iris filtering:

```bat
Tools\RunLocalBinariesMultiplay.bat --iris
```

실행 조건:

| RunId | Mode | RoleFiltering | Timestamp | Detail actors | Summary actors | RunDuration | Metrics start/duration |
|-------|------|---------------|-----------|---------------|----------------|-------------|------------------------|
| ScenarioA_20260601_201201 | Generic | false | `2026-06-01T20:12:03.398Z` | Sensor 60, Drone 30, SupplyCrate 30, total 120 | 1 | 95.000 | 35.000 / 60.000 |
| ScenarioA_20260601_201615 | Iris | true | `2026-06-01T20:16:16.981Z` | Sensor 60, Drone 30, SupplyCrate 30, total 120 | 1 | 95.000 | 35.000 / 60.000 |
| ScenarioA_20260601_203950 | Generic | false | `2026-06-01T20:39:52.023Z` | Sensor 60, Drone 30, SupplyCrate 30, total 120 | 1 | 95.000 | 35.000 / 60.000 |
| ScenarioA_20260601_204211 | Iris | true | `2026-06-01T20:42:12.466Z` | Sensor 60, Drone 30, SupplyCrate 30, total 120 | 1 | 95.000 | 35.000 / 60.000 |

Client snapshot:

| RunId | Role | Zone | SensorCount | DroneCount | SupplyCrateCount | DetailActorTotal | SummaryCount | DetailMaxSequence |
|-------|------|------|-------------|------------|------------------|------------------|--------------|-------------------|
| ScenarioA_20260601_201201 | Commander | -1 | 60 | 30 | 30 | 120 | 1 | 55 |
| ScenarioA_20260601_201201 | FieldAgent | 0 | 60 | 30 | 30 | 120 | 1 | 55 |
| ScenarioA_20260601_201201 | Spectator | -1 | 60 | 30 | 30 | 120 | 1 | 55 |
| ScenarioA_20260601_201615 | Commander | -1 | 60 | 30 | 30 | 120 | 1 | 54 |
| ScenarioA_20260601_201615 | FieldAgent | 0 | 20 | 10 | 10 | 40 | 0 | 54 |
| ScenarioA_20260601_201615 | Spectator | -1 | 0 | 0 | 0 | 0 | 1 | 0 |
| ScenarioA_20260601_203950 | Commander | -1 | 60 | 30 | 30 | 120 | 1 | 54 |
| ScenarioA_20260601_203950 | FieldAgent | 0 | 60 | 30 | 30 | 120 | 1 | 54 |
| ScenarioA_20260601_203950 | Spectator | -1 | 60 | 30 | 30 | 120 | 1 | 54 |
| ScenarioA_20260601_204211 | Commander | -1 | 60 | 30 | 30 | 120 | 1 | 53 |
| ScenarioA_20260601_204211 | FieldAgent | 0 | 20 | 10 | 10 | 40 | 0 | 53 |
| ScenarioA_20260601_204211 | Spectator | -1 | 0 | 0 | 0 | 0 | 1 | 0 |

반복 측정에서도 Generic은 세 role 모두 detail actor 120개와 summary actor 1개를 수신했고, Iris filtering은 Commander 120개, FieldAgent zone 0 detail 40개, Spectator detail 0개 패턴을 유지했다. `ScenarioA_20260601_203950`은 서버가 남은 client 하나보다 먼저 종료된 정황이 있었지만, 자동 client snapshot 3개와 role별 61 samples가 모두 남았으므로 마지막 반복 측정으로 포함한다.

## 9. 결과 표

12 actor baseline은 control run으로 보존한다. 이 작은 actor 수 조건에서는 Iris filtering actor count는 기대대로 줄었지만 connection별 bandwidth 평균은 감소하지 않았다.

| RunId | Mode | RoleFiltering | Role | DetailActorTotal | SummaryCount | AvgOutBytesPerSecond | DeltaOutTotalBytes | Samples | Status |
|-------|------|---------------|------|------------------|--------------|----------------------|--------------------|---------|--------|
| ScenarioA_GenericBaseline_001 | Generic | false | Commander | 12 | 1 | 1516.03 | 174212 | 120 | Recorded |
| ScenarioA_GenericBaseline_001 | Generic | false | FieldAgent | 12 | 1 | 1501.20 | 174606 | 120 | Recorded |
| ScenarioA_GenericBaseline_001 | Generic | false | Spectator | 12 | 1 | 1500.38 | 174460 | 120 | Recorded |
| ScenarioA_IrisFiltering_001 | Iris | true | Commander | 12 | 1 | 1651.53 | 72402 | 45 | Actor count expected, bandwidth not lower |
| ScenarioA_IrisFiltering_001 | Iris | true | FieldAgent | 4 | 0 | 1593.84 | 70055 | 45 | Actor count expected, bandwidth not lower |
| ScenarioA_IrisFiltering_001 | Iris | true | Spectator | 0 | 1 | 1577.47 | 69418 | 45 | Actor count expected, bandwidth not lower |

Avg bandwidth comparison:

| Role | Generic AvgOutBytesPerSecond | Iris AvgOutBytesPerSecond | Difference | Difference % |
|------|------------------------------|---------------------------|------------|--------------|
| Commander | 1516.03 | 1651.53 | +135.50 | +8.9% |
| FieldAgent | 1501.20 | 1593.84 | +92.64 | +6.2% |
| Spectator | 1500.38 | 1577.47 | +77.09 | +5.1% |

120 actor heavy result:

| RunId | Mode | RoleFiltering | Role | DetailActorTotal | SummaryCount | AvgOutBytesPerSecond | DeltaOutTotalBytes | Samples | Status |
|-------|------|---------------|------|------------------|--------------|----------------------|--------------------|---------|--------|
| ScenarioA_GenericHeavy_002 | Generic | false | Commander | 120 | 1 | 2280.28 | 136124 | 61 | Recorded |
| ScenarioA_GenericHeavy_002 | Generic | false | FieldAgent | 120 | 1 | 2307.00 | 139804 | 61 | Recorded |
| ScenarioA_GenericHeavy_002 | Generic | false | Spectator | 120 | 1 | 2263.84 | 135830 | 61 | Recorded |
| ScenarioA_IrisHeavy_002 | Iris | true | Commander | 120 | 1 | 2093.87 | 126350 | 61 | Bandwidth lower |
| ScenarioA_IrisHeavy_002 | Iris | true | FieldAgent | 40 | 0 | 1740.18 | 104396 | 61 | Actor count expected, bandwidth lower |
| ScenarioA_IrisHeavy_002 | Iris | true | Spectator | 0 | 1 | 1571.51 | 94288 | 61 | Actor count expected, bandwidth lower |

120 actor avg bandwidth comparison:

| Role | Generic AvgOutBytesPerSecond | Iris AvgOutBytesPerSecond | Difference | Difference % |
|------|------------------------------|---------------------------|------------|--------------|
| Commander | 2280.28 | 2093.87 | -186.41 | -8.2% |
| FieldAgent | 2307.00 | 1740.18 | -566.82 | -24.6% |
| Spectator | 2263.84 | 1571.51 | -692.33 | -30.6% |

120 actor NetDriver total:

| RunId | Window | Samples | NetDriver DeltaOutTotalBytes |
|-------|--------|---------|------------------------------|
| ScenarioA_GenericHeavy_002 | `2026-05-30T18:22:54.087Z` ~ `2026-05-30T18:23:54.062Z` | 61 | 411758 |
| ScenarioA_IrisHeavy_002 | `2026-05-30T18:26:07.435Z` ~ `2026-05-30T18:27:07.426Z` | 61 | 325034 |

Heavy NetDriver delta comparison:

| Generic DeltaOutTotalBytes | Iris DeltaOutTotalBytes | Difference | Difference % |
|----------------------------|-------------------------|------------|--------------|
| 411758 | 325034 | -86724 | -21.1% |

2026-06-01 120 actor repeat result:

| RunId | Mode | RoleFiltering | Role | DetailActorTotal | SummaryCount | AvgOutBytesPerSecond | DeltaOutTotalBytes | Samples | Status |
|-------|------|---------------|------|------------------|--------------|----------------------|--------------------|---------|--------|
| ScenarioA_20260601_201201 | Generic | false | Commander | 120 | 1 | 2338.31 | 139792 | 61 | Recorded |
| ScenarioA_20260601_201201 | Generic | false | FieldAgent | 120 | 1 | 2338.30 | 139792 | 61 | Recorded |
| ScenarioA_20260601_201201 | Generic | false | Spectator | 120 | 1 | 2337.30 | 139745 | 61 | Recorded |
| ScenarioA_20260601_201615 | Iris | true | Commander | 120 | 1 | 2109.62 | 126856 | 61 | Bandwidth lower |
| ScenarioA_20260601_201615 | Iris | true | FieldAgent | 40 | 0 | 1755.75 | 105421 | 61 | Actor count expected, bandwidth lower |
| ScenarioA_20260601_201615 | Iris | true | Spectator | 0 | 1 | 1579.67 | 94831 | 61 | Actor count expected, bandwidth lower |
| ScenarioA_20260601_203950 | Generic | false | Commander | 120 | 1 | 2334.49 | 139797 | 61 | Recorded |
| ScenarioA_20260601_203950 | Generic | false | FieldAgent | 120 | 1 | 2333.87 | 139831 | 61 | Recorded |
| ScenarioA_20260601_203950 | Generic | false | Spectator | 120 | 1 | 2334.34 | 139820 | 61 | Recorded |
| ScenarioA_20260601_204211 | Iris | true | Commander | 120 | 1 | 2097.70 | 126506 | 61 | Bandwidth lower |
| ScenarioA_20260601_204211 | Iris | true | FieldAgent | 40 | 0 | 1761.10 | 105470 | 61 | Actor count expected, bandwidth lower |
| ScenarioA_20260601_204211 | Iris | true | Spectator | 0 | 1 | 1577.85 | 94731 | 61 | Actor count expected, bandwidth lower |

2026-06-01 avg bandwidth comparison:

| Role | Generic AvgOutBytesPerSecond | Iris AvgOutBytesPerSecond | Difference | Difference % |
|------|------------------------------|---------------------------|------------|--------------|
| Commander | 2338.31 | 2109.62 | -228.69 | -9.8% |
| FieldAgent | 2338.30 | 1755.75 | -582.54 | -24.9% |
| Spectator | 2337.30 | 1579.67 | -757.62 | -32.4% |

2026-06-01 NetDriver delta comparison:

| Generic DeltaOutTotalBytes | Iris DeltaOutTotalBytes | Difference | Difference % |
|----------------------------|-------------------------|------------|--------------|
| 419329 | 327108 | -92221 | -22.0% |

2026-06-01 final repeat avg bandwidth comparison:

| Role | Generic AvgOutBytesPerSecond | Iris AvgOutBytesPerSecond | Difference | Difference % |
|------|------------------------------|---------------------------|------------|--------------|
| Commander | 2334.49 | 2097.70 | -236.79 | -10.1% |
| FieldAgent | 2333.87 | 1761.10 | -572.77 | -24.5% |
| Spectator | 2334.34 | 1577.85 | -756.49 | -32.4% |

2026-06-01 final repeat NetDriver delta comparison:

| Generic DeltaOutTotalBytes | Iris DeltaOutTotalBytes | Difference | Difference % |
|----------------------------|-------------------------|------------|--------------|
| 419448 | 326707 | -92741 | -22.1% |

3회 heavy 평균:

| Role/Scope | Generic average | Generic stdev | Iris average | Iris stdev | Difference | Difference % |
|------------|-----------------|---------------|--------------|-----------|------------|--------------|
| Commander AvgOutBytesPerSecond | 2317.69 | 32.46 | 2100.40 | 8.21 | -217.30 | -9.4% |
| FieldAgent AvgOutBytesPerSecond | 2326.39 | 16.94 | 1752.34 | 10.87 | -574.05 | -24.7% |
| Spectator AvgOutBytesPerSecond | 2311.83 | 41.58 | 1576.34 | 4.28 | -735.48 | -31.8% |
| NetDriver DeltaOutTotalBytes | 416845.00 | 4405.87 | 326283.00 | 1100.09 | -90562.00 | -21.7% |

## 10. 현재 해석

12 actor baseline 기대값:

- Commander, FieldAgent, Spectator 모두 detail actor 12개 수신
- role별 outgoing bandwidth가 대체로 비슷해야 함

12 actor baseline 관찰값:

- 세 role 모두 detail actor 12개와 summary actor 1개를 수신했다.
- 안정 구간의 role별 `AvgOutBytesPerSecond`는 약 1500~1516 bytes/sec로 거의 동일하다.
- 이는 filtering off control run으로서 정상적인 기준값이다. Spectator와 FieldAgent도 모든 detail actor를 받았기 때문에, Iris filtering run에서 이 값과 actor count가 줄어드는지 비교한다.

12 actor Iris filtering 기대값:

- Commander: detail actor 12개 수신
- FieldAgent: 자기 zone detail actor 4개 수신
- Spectator: detail actor 0개, summary actor 1개 수신
- FieldAgent와 Spectator의 connection별 outgoing bandwidth가 Generic baseline보다 낮아야 함

12 actor Iris filtering 관찰값:

- Commander는 detail actor 12개와 summary actor 1개를 수신했다.
- FieldAgent는 자기 zone detail actor 4개만 수신했고 summary actor는 수신하지 않았다.
- Spectator는 detail actor 0개와 summary actor 1개만 수신했다.
- 따라서 role-based actor filtering은 기대대로 동작했다.
- 하지만 connection별 `AvgOutBytesPerSecond`는 Commander, FieldAgent, Spectator 모두 Generic baseline보다 높게 측정됐다.
- 이번 측정만으로는 Iris filtering이 outgoing bandwidth를 줄였다고 말할 수 없다.
- 가능한 원인은 작은 actor 수, 짧고 서로 다른 안정 구간, Iris/connection 초기 비용, 현재 metric이 actor payload 감소보다 고정 overhead에 더 민감한 점이다. 다음 측정에서는 더 긴 동일 window, 더 많은 detail actor, 초기 replication 제외 구간을 별도로 비교한다.

120 actor heavy 기대값:

- Generic heavy: Commander, FieldAgent, Spectator 모두 detail actor 120개와 summary actor 1개 수신
- Iris heavy Commander: detail actor 120개와 summary actor 1개 수신
- Iris heavy FieldAgent: 자기 zone detail actor 40개만 수신
- Iris heavy Spectator: detail actor 0개와 summary actor 1개만 수신
- FieldAgent와 Spectator의 connection별 outgoing bandwidth가 Generic heavy보다 낮아야 함

120 actor heavy 관찰값:

- Generic heavy는 세 번 모두 세 role이 detail actor 120개와 summary actor 1개를 수신했다.
- Iris heavy Commander는 세 번 모두 detail actor 120개와 summary actor 1개를 수신했다.
- Iris heavy FieldAgent는 세 번 모두 자기 zone detail actor 40개만 수신했고 summary actor는 수신하지 않았다.
- Iris heavy Spectator는 세 번 모두 detail actor 0개와 summary actor 1개만 수신했다.
- role-based actor filtering은 120 actor 조건의 반복 측정에서도 기대대로 동작했다.
- 3회 heavy 평균 기준 `AvgOutBytesPerSecond`는 Generic 대비 Commander -9.4%, FieldAgent -24.7%, Spectator -31.8%로 낮아졌다.
- NetDriver 기준 60초 window의 `DeltaOutTotalBytes`도 3회 평균 416845.00에서 326283.00으로 줄어 -21.7% 차이를 보였다.
- 따라서 현재 heavy 조건에서는 Iris role-based filtering이 수신 actor count 감소와 함께 서버 outgoing bandwidth 감소로도 이어졌다고 기록한다.

주의:

- `OutBytesPerSecond`는 순간값이므로 한 줄만 보고 결론을 내리지 않는다.
- 접속 직후 초기 replication 구간은 handshake와 initial spawn 비용이 섞인다.
- 비교할 때는 같은 duration의 안정 구간 평균 또는 같은 run window의 `OutTotalBytes` 증가량을 우선 사용한다.

## 11. 다음 작업

1. Scenario A bandwidth 반복 측정은 3쌍으로 마감한다.
2. Commander에서도 -9.4% 평균 감소가 나온 원인은 다음 profiling 작업에서 별도 확인한다. 같은 actor count를 받는 role이므로 Iris 자체 비용/Generic 경로 차이, window 시작 시점, 패킷/번치 구성 차이를 분리해 본다.
3. Unreal Insights / CSV / NetTrace 중 최소 1개 수집 경로는 다음 단계의 profiling 작업으로 넘긴다.
4. 다음 단계에서는 UObject subobject replication 또는 seamless travel 검증으로 확장하되, 동일한 RunId 산출물 기준을 유지한다.
