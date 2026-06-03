# Scenario A Profiling Capture Path

이 문서는 Scenario A에서 Unreal Insights / CSV Profiling / NetTrace 중 최소 1개 수집 경로를 고정하기 위한 문서다. 여기서 "수집 경로"는 세 도구 중 하나를 선택한다는 뜻이 아니라, 실행 명령, 산출물 위치, 확인 방법을 재현 가능한 절차로 정한다는 뜻이다.

## 1. 도구 차이

| 도구 | 산출물 | 강점 | 한계 | Scenario A 적합도 |
|------|--------|------|------|-------------------|
| Unreal Insights | `.utrace` | Timing, Frame, CPU scope, Log, Net Insights를 한 파일에서 본다. 이미 익숙한 UI를 그대로 쓸 수 있다. | 처음에는 어떤 trace channel을 켤지 정해야 한다. 파일이 커질 수 있다. | 가장 적합 |
| CSV Profiling | `.csv` 또는 `.csv.gz` | 가볍고 반복 측정 평균/p95 표를 만들기 쉽다. CI나 스크립트 집계에 좋다. | 네트워크 object/packet 원인 추적은 Insights보다 약하다. 이미 자체 `server_network_metrics.csv`가 있어 중복될 수 있다. | 보조 수단 |
| NetTrace | `.utrace` 안의 `net` trace data | packet, bunch, replicated object 단위 원인 추적에 가장 직접적이다. Commander bandwidth 감소 원인 분석에 유용하다. | 단독 파일 포맷이라기보다 Unreal Insights의 Net Insights에서 분석하는 데이터에 가깝다. | Insights와 함께 사용 |

선정:

- 1차 수집 경로는 **Unreal Insights trace file + NetTrace channel**로 한다.
- 이유는 사용자가 이미 Unreal Insights에 익숙하고, Scenario A의 다음 질문이 bandwidth 결과의 원인 분리이기 때문이다.
- CSV Profiling은 이후 frame time/p95 자동 표가 필요해질 때 추가한다.

## 2. 수집 범위

처음에는 dedicated server만 trace한다.

이유:

- Scenario A의 성능 결론은 서버 outgoing bandwidth와 connection별 filtering 결과가 중심이다.
- client 3개와 server가 모두 같은 trace file에 쓰면 파일 충돌이 날 수 있다.
- 서버 trace 하나만 있어도 `server_network_metrics.csv`, `client_snapshots.csv`, `run.json`과 같은 RunId로 대조할 수 있다.

수집할 trace channel:

```text
cpu,frame,bookmark,log,net
```

추가 인자:

```text
-NetTrace=1
```

## 3. 실행 명령

`Tools\RunLocalBinariesMultiplay.bat`에 `--trace-server` 옵션을 추가했다. 이 옵션은 server process에만 trace 인자를 붙이고, client process에는 붙이지 않는다.

Generic trace run:

```bat
Tools\RunLocalBinariesMultiplay.bat --no-iris --trace-server --auto-close --run-id ScenarioA_GenericTrace_001
```

Iris trace run:

```bat
Tools\RunLocalBinariesMultiplay.bat --iris --trace-server --auto-close --run-id ScenarioA_IrisTrace_001
```

실제 서버에 붙는 trace 인자:

```text
-trace=cpu,frame,bookmark,log,net -tracefile="<Project>/Saved/ScenarioA/Runs/<RunId>/server_trace.utrace" -tracefiletrunc -NetTrace=1
```

`--auto-close`는 server/client 공통으로 아래 인자를 추가한다.

```text
-ScenarioAAutoExit=1 -ScenarioAAutoExitGraceSeconds=5
```

서버는 `max(ScenarioARunDuration, ScenarioANetworkMetricsStartDelay + ScenarioANetworkMetricsDuration) + Grace` 이후 정상 종료를 요청한다. 클라이언트는 `ScenarioAAutoSnapshotDelay`에 snapshot CSV를 기록한 뒤 Grace 이후 정상 종료를 요청한다.

## 4. 산출물 위치

서버 trace file:

```text
Saved/ScenarioA/Runs/<RunId>/server_trace.utrace
```

기존 Scenario A 산출물:

```text
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/<RunId>/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/<RunId>/server_network_metrics.csv
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/<RunId>/client_snapshots.csv
```

주의:

- `server_trace.utrace`는 배치 파일이 지정한 프로젝트 루트의 `Saved/ScenarioA/Runs/<RunId>/` 아래에 남긴다.
- packaged server의 `run.json`과 `server_network_metrics.csv`는 기존처럼 `Saved/Cooked/WindowsServer/...` 아래에 남을 수 있다.
- 같은 RunId를 기준으로 세 파일 묶음을 대조한다.

## 5. Unreal Insights 확인 절차

1. `G:\UE\UnrealEngine-5.7.1-release\Engine\Binaries\Win64\UnrealInsights.exe`를 실행한다.
2. `server_trace.utrace`를 연다.
3. Timing Insights에서 60초 metrics window와 겹치는 구간을 본다.
4. Net Insights에서 packet/object 관련 이벤트를 확인한다.
5. `PerformanceRuns.md`에는 원본 수치를 옮기지 말고, trace에서 확인한 해석만 요약한다.

우선 확인할 질문:

- Commander는 Generic/Iris 모두 detail actor 120개를 받는데도 왜 Iris run의 outgoing bandwidth가 낮았는가?
- FieldAgent/Spectator에서 actor count 감소가 packet/object 단위 감소와 같은 방향으로 보이는가?
- 초기 replication 구간과 60초 안정 window의 packet 구성이 다른가?

## 6. 판정 기준

Trace run은 아래 조건을 만족해야 유효하다.

- `run.json`이 있다.
- `client_snapshots.csv`가 있다.
- `server_network_metrics.csv`가 있다.
- `server_trace.utrace`가 있다.
- `run.json`의 `mode`, `roleFiltering`, `detailActorTotal`, `networkMetricsStartDelay`, `networkMetricsDuration`이 비교 조건과 일치한다.

처음 trace run은 성능 결론을 새로 쓰기 위한 run이 아니라, 기존 3회 heavy 평균에서 남은 원인 분석 질문을 좁히기 위한 run으로 다룬다.

## 7. 2026-06-03 trace run 확인

Unreal Insights trace run 1쌍을 실행하고 `server_trace.utrace`를 열어 Networking Insights의 Packet/Packet Content 화면을 확인했다.

| RunId | Mode | RoleFiltering | Trace file | Trace size | Server metrics | 판정 |
|-------|------|---------------|------------|------------|----------------|------|
| `ScenarioA_20260603_134820` | Generic | false | `Saved/ScenarioA/Runs/ScenarioA_20260603_134820/server_trace.utrace` | 6,687,000 bytes | 3 connections, 61 samples | Trace captured |
| `ScenarioA_20260603_143017` | Iris | true | `Saved/ScenarioA/Runs/ScenarioA_20260603_143017/server_trace.utrace` | 9,560,605 bytes | 3 connections, 61 samples | Trace captured |

서버 metrics 요약:

| RunId | Role/Scope | AvgOutBytesPerSecond | DeltaOutTotalBytes | Samples |
|-------|------------|----------------------|--------------------|---------|
| `ScenarioA_20260603_134820` | Commander | 2337.15 | 139811 | 61 |
| `ScenarioA_20260603_134820` | FieldAgent | 2337.13 | 139810 | 61 |
| `ScenarioA_20260603_134820` | Spectator | 2338.02 | 139781 | 61 |
| `ScenarioA_20260603_134820` | NetDriver | n/a | 419402 | 61 |
| `ScenarioA_20260603_143017` | Commander | 2119.90 | 126928 | 61 |
| `ScenarioA_20260603_143017` | FieldAgent | 1760.61 | 105518 | 61 |
| `ScenarioA_20260603_143017` | Spectator | 1578.23 | 94718 | 61 |
| `ScenarioA_20260603_143017` | NetDriver | n/a | 327164 | 61 |

이번 trace run의 server metrics는 기존 120 actor heavy 3회 측정과 같은 방향이다. Generic은 세 role의 bandwidth가 거의 같고, Iris는 FieldAgent/Spectator의 outgoing bandwidth가 낮다.

주의:

- `client_snapshots.csv`는 두 run 모두 `NetMode=Standalone`, `Role=Commander`로 기록됐다.
- 서버 metrics에는 `ConnectionCount=3`과 Commander/FieldAgent/Spectator connection row가 모두 있으므로 server-side trace capture 자체는 유효하다.
- 이번 trace run의 client snapshot은 actor count 판정에는 쓰지 않고, trace capture와 server metrics 확인용으로만 다룬다.
