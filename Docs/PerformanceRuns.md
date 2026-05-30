# Scenario A Performance Runs

이 문서는 Scenario A의 actor count와 outgoing bandwidth 측정 결과를 같은 RunId 기준으로 정리하기 위한 문서다.

핵심 목적은 “Iris filtering으로 수신 actor 수가 줄었다”에서 멈추지 않고, 같은 조건에서 서버 outgoing bandwidth도 함께 달라졌는지 확인하는 것이다.

## 1. 데이터 준비 상태

2026-05-30 기준 Generic baseline과 Iris filtering run을 모두 확보했다.

현재 상태:

- Iris filtering run: client snapshot CSV와 server bandwidth CSV 확보됨
- Generic baseline run: client snapshot CSV와 server bandwidth CSV 확보됨
- `ScenarioA_GenericHeavy_001` / `ScenarioA_IrisHeavy_001`: 실행은 완료됐지만 오래된 Server/Client 바이너리로 실행되어 heavy 측정으로는 무효
- 결론: role-based filtering에 따른 actor count 감소는 확인했다. 단, 이번 run의 connection별 outgoing bandwidth 평균은 FieldAgent/Spectator에서도 Generic baseline보다 낮지 않았으므로 bandwidth 개선 결론은 내리지 않는다.

필수 비교 run:

| Run type | 필수 상태 | 현재 상태 | 비고 |
|----------|-----------|-----------|------|
| Generic baseline | 필요 | 확보 | `-UseIrisReplication=0`, `ScenarioAEnableRoleFiltering=0`, RunId `ScenarioA_GenericBaseline_001` |
| Iris filtering on | 필요 | 확보 | `-UseIrisReplication=1`, `ScenarioAEnableRoleFiltering=1`, RunId `ScenarioA_IrisFiltering_001` |
| Generic heavy | 필요 | 무효 | RunId `ScenarioA_GenericHeavy_001`, stale binary로 detail actor 12개 조건 실행 |
| Iris heavy | 필요 | 무효 | RunId `ScenarioA_IrisHeavy_001`, stale binary로 detail actor 12개 조건 실행 |

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

`run.json`에서 확인할 것:

- `mode`
- `roleFiltering`
- `seed`
- `sensorCount`
- `droneCount`
- `supplyCrateCount`
- `runDuration`
- `networkMetricsInterval`

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

`OutBytesPerSecond`는 이 안정 구간의 단순 평균이다. `DeltaOutTotalBytes`는 같은 구간에서 마지막 `OutTotalBytes`와 첫 `OutTotalBytes`의 차이다.

NetDriver 전체 row는 `OutBytesPerSecond`가 0으로 기록되므로 현재 해석에는 connection별 값을 우선 사용한다. 두 run의 안정 구간 길이가 다르므로 `DeltaOutTotalBytes`끼리는 직접 비교하지 않고, role별 `AvgOutBytesPerSecond`를 우선 비교한다.

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

무효 판정 후 `Tools\BuildServer.bat`와 `Tools\BuildClient.bat`를 실행해 새 Server/Client 바이너리 빌드는 성공했다. 다음 측정은 같은 RunId를 재사용하지 말고 새 RunId로 다시 실행한다.

## 7. 재측정 명령

120 actor 동일 window 측정은 새로 빌드된 바이너리로 다시 실행해야 한다.

```bat
Tools\RunLocalBinariesMultiplay.bat --no-iris --run-id ScenarioA_GenericHeavy_002
```

Iris filtering:

```bat
Tools\RunLocalBinariesMultiplay.bat --iris --run-id ScenarioA_IrisHeavy_002
```

두 run 모두 완료 후 확인할 것:

```text
Saved/ScenarioA/Runs/ScenarioA_GenericBaseline_001/
Saved/ScenarioA/Runs/ScenarioA_IrisFiltering_001/
Saved/ScenarioA/Runs/ScenarioA_GenericHeavy_002/
Saved/ScenarioA/Runs/ScenarioA_IrisHeavy_002/
```

각 폴더에 `run.json`, `client_snapshots.csv`, `server_network_metrics.csv`가 모두 있어야 한다.

## 8. 결과 표

Generic baseline은 control run으로 기록한다. Iris filtering actor count는 기대대로 줄었지만, 이번 run의 connection별 bandwidth 평균은 감소하지 않았다.

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

## 9. 현재 해석

Generic baseline 기대값:

- Commander, FieldAgent, Spectator 모두 detail actor 12개 수신
- role별 outgoing bandwidth가 대체로 비슷해야 함

Generic baseline 관찰값:

- 세 role 모두 detail actor 12개와 summary actor 1개를 수신했다.
- 안정 구간의 role별 `AvgOutBytesPerSecond`는 약 1500~1516 bytes/sec로 거의 동일하다.
- 이는 filtering off control run으로서 정상적인 기준값이다. Spectator와 FieldAgent도 모든 detail actor를 받았기 때문에, Iris filtering run에서 이 값과 actor count가 줄어드는지 비교한다.

Iris filtering 기대값:

- Commander: detail actor 12개 수신
- FieldAgent: 자기 zone detail actor 4개 수신
- Spectator: detail actor 0개, summary actor 1개 수신
- FieldAgent와 Spectator의 connection별 outgoing bandwidth가 Generic baseline보다 낮아야 함

Iris filtering 관찰값:

- Commander는 detail actor 12개와 summary actor 1개를 수신했다.
- FieldAgent는 자기 zone detail actor 4개만 수신했고 summary actor는 수신하지 않았다.
- Spectator는 detail actor 0개와 summary actor 1개만 수신했다.
- 따라서 role-based actor filtering은 기대대로 동작했다.
- 하지만 connection별 `AvgOutBytesPerSecond`는 Commander, FieldAgent, Spectator 모두 Generic baseline보다 높게 측정됐다.
- 이번 측정만으로는 Iris filtering이 outgoing bandwidth를 줄였다고 말할 수 없다.
- 가능한 원인은 작은 actor 수, 짧고 서로 다른 안정 구간, Iris/connection 초기 비용, 현재 metric이 actor payload 감소보다 고정 overhead에 더 민감한 점이다. 다음 측정에서는 더 긴 동일 window, 더 많은 detail actor, 초기 replication 제외 구간을 별도로 비교한다.

주의:

- `OutBytesPerSecond`는 순간값이므로 한 줄만 보고 결론을 내리지 않는다.
- 접속 직후 초기 replication 구간은 handshake와 initial spawn 비용이 섞인다.
- 비교할 때는 같은 duration의 안정 구간 평균 또는 같은 run window의 `OutTotalBytes` 증가량을 우선 사용한다.

## 10. 다음 작업

1. 새로 빌드된 Server/Client 바이너리로 `ScenarioA_GenericHeavy_002`, `ScenarioA_IrisHeavy_002`를 실행한다.
2. `run.json`에서 `detailActorTotal=120`, `runDuration=95.000`, `networkMetricsStartDelay=35.000`, `networkMetricsDuration=60.000`을 먼저 확인한다.
3. `server_network_metrics.csv`에 `MetricsWindowElapsedSeconds` 컬럼이 있는지 확인한다.
4. `client_snapshots.csv`가 client 쪽 run 폴더에 생성됐는지 확인한다.
5. 유효하면 `MetricsWindowElapsedSeconds` 기준으로 두 run의 같은 길이 window를 비교한다.
6. 결과가 기대와 다르면 `DebuggingNotes.md`에 원인 분석을 남긴다.
