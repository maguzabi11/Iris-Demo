# Scenario A Debugging Notes

이 문서는 Scenario A의 role-based filtering과 성능 측정 과정에서 확인한 문제, 원인, 판정 기준, 남은 확인 항목을 기록한다. 목적은 좋은 결과만 남기는 것이 아니라, 비교 실험이 왜 유효한지와 어떤 데이터는 왜 제외했는지를 설명할 수 있게 만드는 것이다.

관련 문서:

- [ScenarioA_RoleBasedFiltering.md](./ScenarioA_RoleBasedFiltering.md)
- [PerformanceRuns.md](./PerformanceRuns.md)
- [실험기본기.md](./실험기본기.md)

## 1. 현재 결론

Scenario A의 core 기능은 동작한다.

- Generic baseline에서는 Commander, FieldAgent, Spectator가 모두 detail actor를 받는다.
- Iris role filtering에서는 Commander는 전체 detail actor, FieldAgent는 자기 zone detail actor, Spectator는 summary actor만 받는다.
- 12 actor run에서는 actor count 감소는 확인됐지만 bandwidth 감소는 확인되지 않았다.
- 120 actor heavy run에서는 2회 측정 모두 actor count 감소와 outgoing bandwidth 감소가 함께 확인됐다.

현재 가장 중요한 성능 결과는 다음이다.

| 비교 | Generic | Iris | 차이 |
|------|---------|------|------|
| FieldAgent AvgOutBytesPerSecond | 2322.65 | 1747.97 | -24.7% |
| Spectator AvgOutBytesPerSecond | 2300.57 | 1575.59 | -31.5% |
| NetDriver DeltaOutTotalBytes | 415543.50 | 326071.00 | -21.5% |

이 결론은 `ScenarioA_GenericHeavy_002`/`ScenarioA_IrisHeavy_002`와 `ScenarioA_20260601_201201`/`ScenarioA_20260601_201615`의 60초 metrics window 2회 평균 기준이다.

## 2. 유효하지 않은 heavy run 판정

처음 실행한 heavy run은 이름과 실제 조건이 달랐다.

| RunId | 문제 | 판정 |
|-------|------|------|
| ScenarioA_GenericHeavy_001 | detail actor 12개, old CSV schema, client snapshot 없음 | heavy 비교에서 제외 |
| ScenarioA_IrisHeavy_001 | detail actor 12개, old CSV schema, client snapshot 없음 | heavy 비교에서 제외 |

원인:

- `Tools\RunLocalBinariesMultiplay.bat`는 `Binaries\Win64\IrisDemoServer.exe`와 `IrisDemoClient.exe`를 실행한다.
- 당시 바이너리가 새 heavy 기본값 반영 전 빌드였다.
- 따라서 RunId 이름은 heavy였지만 실제 실행 조건은 heavy가 아니었다.

대응:

- `run.json`의 `detailActorTotal`, `runDuration`, `networkMetricsStartDelay`, `networkMetricsDuration`을 먼저 확인하는 절차를 문서화했다.
- 같은 RunId를 재사용하지 않고 `_002` RunId로 재측정했다.
- 무효 run의 수치는 참고값으로만 남기고 결론 표에서는 제외했다.

교훈:

- RunId 이름이나 실행 의도를 믿지 않는다.
- 비교 실험은 항상 원본 산출물의 실제 조건으로 판정한다.

## 3. Generic/Iris 실행 모드 확인 이슈

IrisDemo의 기본 replication mode는 Iris다. Generic은 기본값이 아니라 비교용 control run이다.

확인 기준:

- Generic run: `-UseIrisReplication=0`, `-net.Iris.UseIrisReplication=0`, `ScenarioAEnableRoleFiltering=0`
- Iris run: `-UseIrisReplication=1`, `-net.Iris.UseIrisReplication=1`, `ScenarioAEnableRoleFiltering=1`

주의할 점:

- UE 로그에서 CVar 값만 보고 Iris 여부를 판정하면 헷갈릴 수 있다.
- Scenario A 문서에서는 실제 `NetDriver->IsUsingIrisReplication()` 기반의 `Mode` 기록을 우선한다.
- `run.json`의 `mode`, `irisCommandLineOverride`, `roleFiltering`, `commandLine`을 함께 확인한다.

현재 판정:

- `ScenarioA_GenericHeavy_002`는 `mode=Generic`, `roleFiltering=false`로 유효하다.
- `ScenarioA_IrisHeavy_002`는 `mode=Iris`, `roleFiltering=true`로 유효하다.
- `ScenarioA_20260601_201201`는 `mode=Generic`, `roleFiltering=false`로 유효하다.
- `ScenarioA_20260601_201615`는 `mode=Iris`, `roleFiltering=true`로 유효하다.

## 4. 저장 위치 분리 이슈

패키징된 client/server 실행에서는 산출물이 한 폴더에 모이지 않는다.

확인된 위치:

```text
Saved/Cooked/WindowsClient/IrisDemo/Saved/ScenarioA/Runs/<RunId>/client_snapshots.csv
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/<RunId>/run.json
Saved/Cooked/WindowsServer/IrisDemo/Saved/ScenarioA/Runs/<RunId>/server_network_metrics.csv
```

주의할 점:

- server 폴더만 보면 client snapshot이 없는 것처럼 보일 수 있다.
- client snapshot은 각 local client가 자기 실행 환경의 Saved 아래에 남긴다.
- 성능 비교 문서에서는 client/server 산출물을 같은 RunId 기준으로 수동 대조한다.

남은 개선 후보:

- Run 종료 후 client/server 산출물을 한 위치로 모으는 copy/collect 스크립트를 만든다.
- 또는 `Tools\RunLocalBinariesMultiplay.bat`가 실행 종료 후 RunId별 manifest를 생성하게 한다.

## 5. 12 actor run에서 bandwidth가 줄지 않은 이유

12 actor run에서 Iris filtering은 actor count를 기대대로 줄였다.

| Role | Generic DetailActorTotal | Iris DetailActorTotal |
|------|--------------------------|-----------------------|
| Commander | 12 | 12 |
| FieldAgent | 12 | 4 |
| Spectator | 12 | 0 |

하지만 connection별 `AvgOutBytesPerSecond`는 오히려 Iris run에서 높았다.

가능한 원인:

- actor 수가 작아 actor payload 감소보다 고정 overhead가 더 크게 보였을 수 있다.
- Generic/Iris run의 안정 구간 길이가 달랐다.
- connection 초기 비용, handshake, initial spawn 영향이 충분히 제거되지 않았을 수 있다.
- Iris와 Generic의 packet/bunch 구성 차이가 작은 조건에서는 filtering 효과보다 더 크게 보였을 수 있다.

판정:

- 12 actor run은 actor filtering 기능 검증용으로는 유효하다.
- bandwidth 개선 결론에는 사용하지 않는다.
- bandwidth 결론은 120 actor heavy run을 기준으로 한다.

## 6. 120 actor heavy run의 유효성

`ScenarioA_GenericHeavy_002`/`ScenarioA_IrisHeavy_002`와 `ScenarioA_20260601_201201`/`ScenarioA_20260601_201615`는 비교 조건을 만족한다.

공통 조건:

- Seed: 1001
- Sensor: 60
- Drone: 30
- SupplyCrate: 30
- DetailActorTotal: 120
- SummaryCount: 1
- RunDuration: 95.000
- NetworkMetricsStartDelay: 35.000
- NetworkMetricsDuration: 60.000
- Samples per role: 61

Client snapshot 결과:

| RunId | Role | DetailActorTotal | SummaryCount |
|-------|------|------------------|--------------|
| ScenarioA_GenericHeavy_002 | Commander | 120 | 1 |
| ScenarioA_GenericHeavy_002 | FieldAgent | 120 | 1 |
| ScenarioA_GenericHeavy_002 | Spectator | 120 | 1 |
| ScenarioA_IrisHeavy_002 | Commander | 120 | 1 |
| ScenarioA_IrisHeavy_002 | FieldAgent | 40 | 0 |
| ScenarioA_IrisHeavy_002 | Spectator | 0 | 1 |
| ScenarioA_20260601_201201 | Commander | 120 | 1 |
| ScenarioA_20260601_201201 | FieldAgent | 120 | 1 |
| ScenarioA_20260601_201201 | Spectator | 120 | 1 |
| ScenarioA_20260601_201615 | Commander | 120 | 1 |
| ScenarioA_20260601_201615 | FieldAgent | 40 | 0 |
| ScenarioA_20260601_201615 | Spectator | 0 | 1 |

결론:

- Generic heavy는 두 번 모두 control run으로 유효하다.
- Iris heavy는 두 번 모두 role-based filtering 결과가 기대값과 일치한다.
- FieldAgent와 Spectator는 actor count 감소와 bandwidth 감소가 함께 나타났다.

## 7. Commander bandwidth 감소는 추가 확인 필요

120 actor heavy run에서 Commander는 Generic과 Iris 모두 detail actor 120개와 summary actor 1개를 받았다. 그런데 2회 측정 모두 `AvgOutBytesPerSecond`는 Iris에서 더 낮았다.

| Role | Generic AvgOutBytesPerSecond | Iris AvgOutBytesPerSecond | 차이 |
|------|------------------------------|---------------------------|------|
| Commander | 2280.28 | 2093.87 | -8.2% |
| Commander repeat | 2338.31 | 2109.62 | -9.8% |
| Commander 2회 평균 | 2309.30 | 2101.75 | -9.0% |

이 결과는 나쁜 결과가 아니라 추가 질문이다.

가능한 원인:

- Generic과 Iris replication path 자체의 packet/bunch 구성 차이
- 측정 window 시작 시점의 미세한 차이
- Iris 경로의 serialization 또는 scheduling 차이
- actor count는 같지만 property update timing이 다르게 분포했을 가능성
- 측정 run 수가 아직 2쌍뿐이라 편차를 안정적으로 말하기 어려움

현재 판정:

- Commander 감소는 "role filtering으로 actor count가 줄어서"라고 설명하면 안 된다.
- FieldAgent/Spectator 감소와 구분해서 별도 관찰값으로 기록한다.
- 반복 run과 NetTrace/CSV profiling으로 추가 확인한다.

## 8. 남은 Scenario A 작업

Scenario A를 기능 데모가 아니라 포트폴리오용 재현 가능한 샘플로 마감하려면 아래 작업이 남아 있다.

### 반드시 마무리할 작업

1. Scenario A 마무리 문서 보강
   - 현재 상태, A6/A6.5/A7 체크리스트, heavy run 결과 표는 2026-06-01 기준으로 2회 측정까지 갱신했다.
   - 아직 구현 class 목록, 실행 방법, UE 5.7.1 Iris 제약/주의점, 다음 시나리오로 넘길 TODO를 한 섹션으로 정리해야 한다.

2. 반복 측정
   - 같은 heavy 조건을 1회 더 반복해 최소 3쌍을 확보한다.
   - FieldAgent/Spectator bandwidth 감소 방향이 유지되는지 확인한다.
   - 평균과 편차를 `PerformanceRuns.md`에 추가한다.

3. Unreal Insights / CSV / NetTrace 중 최소 1개 수집 경로 작성
   - 지금은 자체 CSV인 `server_network_metrics.csv`가 있다.
   - 포트폴리오 설득력을 위해 UE 도구 기반 산출물 수집 절차도 하나는 남겨야 한다.

4. Scenario A 실행법 정리
   - build, server/client 실행, Generic/Iris 비교 명령, 산출물 위치를 한 섹션으로 정리한다.
   - 제3자가 문서만 보고 재현할 수 있어야 한다.

5. UE 5.7.1 Iris 제약/주의점 반영
   - `TargetRules.bUseIris` 미존재
   - `SetupIrisSupport(Target)` 유지
   - `InitGame` URL option과 command line option 차이
   - CVar 로그보다 `NetDriver->IsUsingIrisReplication()` 판정을 우선한 점
   - 패키징 client/server의 Saved 경로 분리

### 코드로 남은 작업

1. actor count/zone 배치 seed 설정 분리
   - 현재 기본값 기반 실행은 가능하지만 문서상 TODO로 남아 있다.
   - Scenario A 재현성을 더 높이려면 설정 경로를 명확히 한다.

2. 역할 변경 시 filter membership 갱신 검증
   - 현재 자동 role 할당과 spawn 시 membership 갱신은 확인됐다.
   - runtime role 변경 console command가 들어가면 group status가 즉시 갱신되는지 검증해야 한다.

3. actor despawn 시 filter membership 정리
   - 현재 Scenario A는 deterministic spawn 중심이라 큰 문제는 없지만, 데모 완성도 관점에서는 group에서 제거하는 경로를 확인해야 한다.

4. Spectator summary update cadence 지연 처리
   - 현재 Spectator는 detail actor를 받지 않고 summary만 받는다.
   - 기획상 "지연되거나 축약된 summary"를 보여주려면 update cadence를 늦추는 처리가 남아 있다.

### 있으면 좋은 작업

1. Run 산출물 collect 스크립트
   - client/server에 흩어진 `run.json`, `client_snapshots.csv`, `server_network_metrics.csv`를 한 폴더로 모은다.

2. replicated object count 또는 relevant object count 추가
   - bandwidth 외에 Iris filtering의 직접적인 object-level 지표를 추가하면 설득력이 커진다.

3. false positive/false negative 목록 자동 기록
   - Spectator가 detail actor를 하나라도 받으면 바로 실패로 표시하는 검증 CSV를 만들 수 있다.

4. Commander bandwidth 감소 원인 분석
   - 반복 run, NetTrace, packet/bunch count로 원인을 분리한다.

## 9. Scenario A 마감 기준

Scenario A는 아래 조건을 만족하면 "마감"으로 본다.

- 문서만 보고 Generic/Iris 비교 실행을 재현할 수 있다.
- run 산출물의 위치와 의미가 설명되어 있다.
- Generic baseline과 Iris filtering의 actor count 차이를 표로 설명할 수 있다.
- 120 actor heavy 조건에서 bandwidth 감소를 수치로 설명할 수 있다.
- 무효 run과 유효 run을 왜 구분했는지 설명되어 있다.
- 남은 한계가 숨겨지지 않고 다음 시나리오 TODO로 분리되어 있다.

이 기준까지 가면 Scenario A는 "Iris 기능을 써봤다"가 아니라 "Iris filtering을 통제된 조건에서 설계, 실행, 측정, 디버깅했다"는 포트폴리오 샘플로 쓸 수 있다.
