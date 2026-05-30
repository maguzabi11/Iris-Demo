# Scenario A Performance Runs

이 문서는 Scenario A의 actor count와 outgoing bandwidth 측정 결과를 같은 RunId 기준으로 정리하기 위한 문서다.

핵심 목적은 “Iris filtering으로 수신 actor 수가 줄었다”에서 멈추지 않고, 같은 조건에서 서버 outgoing bandwidth도 함께 달라졌는지 확인하는 것이다.

## 1. 데이터 준비 상태

2026-05-30 기준 현재 비교 표는 아직 완성하지 않는다.

현재 상태:

- Iris filtering run: 측정 가능 경로는 준비됨
- Generic baseline run: bandwidth CSV 기준 데이터가 아직 없음
- 결론: Generic baseline이 없으므로 Generic vs Iris bandwidth 비교 표를 작성하면 안 됨

필수 비교 run:

| Run type | 필수 상태 | 현재 상태 | 비고 |
|----------|-----------|-----------|------|
| Generic baseline | 필요 | 누락 | `--no-iris`, `ScenarioAEnableRoleFiltering=0`으로 재측정 필요 |
| Iris filtering on | 필요 | 확인 필요 | `--iris`, `ScenarioAEnableRoleFiltering=1` run 산출물 확인 필요 |

## 2. 유효한 run 판정 기준

하나의 RunId가 성능 비교에 쓰이려면 아래 파일이 모두 있어야 한다.

```text
Saved/ScenarioA/Runs/<RunId>/run.json
Saved/ScenarioA/Runs/<RunId>/client_snapshots.csv
Saved/ScenarioA/Runs/<RunId>/server_network_metrics.csv
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

## 3. Generic baseline 누락 감지

다음 조건을 만족하는 run이 없으면 Generic baseline은 누락 상태로 본다.

```text
run.json:
  mode == "Generic"
  roleFiltering == false

client_snapshots.csv:
  Commander / FieldAgent / Spectator 모두 DetailActorTotal=12

server_network_metrics.csv:
  Scope=Connection row가 role별로 존재
```

현재 문서 기준 Generic baseline은 누락 상태다. 따라서 아래 결과 표는 비워 둔다.

## 4. 필요한 재측정 명령

Generic baseline:

```bat
Tools\RunLocalBinariesMultiplay.bat --no-iris --run-id ScenarioA_GenericBaseline_001 -ScenarioAEnableRoleFiltering=0 -ScenarioARunDuration=30
```

Iris filtering:

```bat
Tools\RunLocalBinariesMultiplay.bat --iris --run-id ScenarioA_IrisFiltering_001 -ScenarioAEnableRoleFiltering=1 -ScenarioARunDuration=30
```

두 run 모두 완료 후 확인할 것:

```text
Saved/ScenarioA/Runs/ScenarioA_GenericBaseline_001/
Saved/ScenarioA/Runs/ScenarioA_IrisFiltering_001/
```

각 폴더에 `run.json`, `client_snapshots.csv`, `server_network_metrics.csv`가 모두 있어야 한다.

## 5. 결과 표

Generic baseline이 아직 없으므로 이 표는 작성 보류 상태다.

| RunId | Mode | RoleFiltering | Role | DetailActorTotal | SummaryCount | AvgOutBytesPerSecond | TotalOutBytes | Status |
|-------|------|---------------|------|------------------|--------------|----------------------|---------------|--------|
| ScenarioA_GenericBaseline_001 | Generic | false | Commander | TBD | TBD | TBD | TBD | Missing |
| ScenarioA_GenericBaseline_001 | Generic | false | FieldAgent | TBD | TBD | TBD | TBD | Missing |
| ScenarioA_GenericBaseline_001 | Generic | false | Spectator | TBD | TBD | TBD | TBD | Missing |
| ScenarioA_IrisFiltering_001 | Iris | true | Commander | TBD | TBD | TBD | TBD | Pending |
| ScenarioA_IrisFiltering_001 | Iris | true | FieldAgent | TBD | TBD | TBD | TBD | Pending |
| ScenarioA_IrisFiltering_001 | Iris | true | Spectator | TBD | TBD | TBD | TBD | Pending |

## 6. 해석 기준

Generic baseline 기대값:

- Commander, FieldAgent, Spectator 모두 detail actor 12개 수신
- role별 outgoing bandwidth가 대체로 비슷해야 함

Iris filtering 기대값:

- Commander: detail actor 12개 수신
- FieldAgent: 자기 zone detail actor 4개 수신
- Spectator: detail actor 0개, summary actor 1개 수신
- FieldAgent와 Spectator의 connection별 outgoing bandwidth가 Generic baseline보다 낮아야 함

주의:

- `OutBytesPerSecond`는 순간값이므로 한 줄만 보고 결론을 내리지 않는다.
- 접속 직후 초기 replication 구간은 handshake와 initial spawn 비용이 섞인다.
- 비교할 때는 같은 duration의 안정 구간 평균 또는 같은 run window의 `OutTotalBytes` 증가량을 우선 사용한다.

## 7. 다음 작업

1. Generic baseline run을 먼저 확보한다.
2. Iris filtering run 산출물이 같은 기준을 만족하는지 확인한다.
3. 두 run의 CSV를 기준으로 결과 표를 채운다.
4. 결과가 기대와 다르면 `DebuggingNotes.md`에 원인 분석을 남긴다.
