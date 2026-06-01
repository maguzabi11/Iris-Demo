# Scenario A 상세 기획: Role-Based Interest Filtering

이 문서는 `Iris Relay Lab`의 첫 번째 구현 단위인 Scenario A를 작은 빌드 가능 단계로 나누고, 진행 상황을 계속 체크하기 위한 작업 문서다.

## 1. 현재 상태

- 기준일: 2026-06-01
- 엔진 기준: UE 5.7.1
- 목표 시나리오: 역할별 관심 정보 차등 복제
- 현재 단계: A3/A4 role별 client snapshot과 120 actor heavy bandwidth 반복 측정을 기록한 상태
- 포트폴리오 관점: 기능 데모 기준으로는 약 85~90% 완료, 성능 측정/디버깅 중심 포트폴리오 기준으로는 약 65~70% 완료

이미 확인한 기반 상태:

- [x] `IrisDemo.uproject`에 `Iris` plugin 활성화
- [x] `Source/IrisDemo/IrisDemo.Build.cs`에서 `SetupIrisSupport(Target)` 유지
- [x] `Source/IrisDemo.Target.cs`, `Source/IrisDemoEditor.Target.cs`에 존재하지 않는 `bUseIris` 설정 없음
- [x] `Config/DefaultEngine.ini`에 `net.Iris.UseIrisReplication=1` 설정
- [x] `Config/DefaultEngine.ini`에 registered subobject list/push model 관련 기본 설정 추가
- [x] Scenario A 전용 C++ class 골격 작성
- [x] 역할 선택/할당 flow 작성
- [x] 역할별 replicated actor visibility 차이 구현
- [x] Generic/Iris 비교 실행 절차 작성
- [x] 측정 UI 또는 로그 요약 구현
- [x] UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 확인
- [x] 120 actor heavy 조건에서 Generic/Iris outgoing bandwidth 반복 측정 기록
- [x] 무효 run과 유효 run의 판정 기준을 `PerformanceRuns.md`, `DebuggingNotes.md`, `실험기본기.md`에 문서화

상위 포트폴리오 보강 방향은 [Portfolio_Performance_Debugging_Plan.md](./Portfolio_Performance_Debugging_Plan.md)를 따른다. Scenario A는 이 계획의 첫 검증 단위이며, 단순히 actor 수가 다르게 보이는 것보다 “같은 조건의 Generic/Iris 실행을 구성하고 role별 false positive 복제 여부를 설명할 수 있는가”를 중점으로 마감한다.

측정 결과는 [PerformanceRuns.md](./PerformanceRuns.md), 비교 실험 기본기는 [실험기본기.md](./실험기본기.md), 문제/무효 run/남은 디버깅 항목은 [DebuggingNotes.md](./DebuggingNotes.md)에 분리해 기록한다.

## 2. 시나리오 목표

Scenario A는 “Replication Graph 없이 Iris Filtering으로 connection별 관심 대상을 제어한다”는 메시지를 보여주는 최소 완성 단위다.

플레이어 역할은 세 가지로 둔다.

| 역할 | 받아야 하는 정보 | 받지 않아야 하는 정보 |
|------|------------------|-----------------------|
| Commander | 전체 squad summary, 전체 sensor event, 전체 objective marker | 개별 현장 요원의 owner-only 내부 상태 |
| Field Agent | 자기 주변 sensor/drone/supply crate, 자기 squad 정보, 자기 objective | 먼 구역 sensor/drone의 상세 상태, commander 전용 전체 요약 |
| Spectator | 지연되거나 축약된 전체 summary | 실시간 상세 sensor/drone 상태, owner-only 상태 |

성공 기준은 화면에 많이 보이는 actor 수가 아니라, connection별로 서버가 보내는 replicated object/relevant object 수가 역할에 따라 다르게 나오는 것이다.

여기서 역할은 actor 자체의 속성이 아니라 player/connection의 속성이다. 테스트 actor에는 `ZoneId`, category, owner, summary/detail 여부 같은 metadata를 둔다. 서버의 filtering policy가 “이 역할을 가진 connection에 이 metadata를 가진 actor를 보낼 것인가?”를 판단한다.

```text
PlayerState.Role = Commander / FieldAgent / Spectator
RelaySensorActor.ZoneId = 2
RelaySensorActor.Category = SensorDetail
FilteringPolicy(Role, ActorMetadata) -> allow / deny
```

## 3. 최소 구현 범위

첫 번째 완성 단위에서는 맵과 UI를 과하게 만들지 않는다. 기본 ThirdPerson 맵 또는 간단한 Operation 테스트 맵 위에서 replicated actor와 로그/디버그 UI만으로 검증한다.

### 포함

- 역할 enum: `Commander`, `FieldAgent`, `Spectator`
- 역할 저장 위치: `PlayerState` 우선
- 테스트 replicated actor:
- `RelaySensorActor`: 구역 id, alert level, last event sequence
- `RelayDroneActor`: 구역 id, battery, state
- `RelaySupplyCrateActor`: 구역 id, stock summary
- `RelaySquadSummaryActor` 또는 GameState summary: commander/spectator용 요약
- 테스트 actor는 데모에서 월드 상 객체로 인식할 수 있도록 최소 visual mesh/component를 가진다.
- connection별 관심 정책:
  - Commander: 전체 operational summary 허용
  - FieldAgent: 자신의 active zone 또는 거리 기준 actor 허용
  - Spectator: summary actor만 허용하고 상세 actor는 제한
- 디버그 표시:
  - local role
  - visible/relevant test object count
  - last received event sequence
  - Generic/Iris 실행 모드

### 제외

- UObject inventory/subobject replication
- custom prioritizer
- seamless travel
- full Unreal Insights 자동 분석
- 대규모 bot stress test

이 항목들은 Scenario C, B, D, E에서 확장한다.

## 4. 구현 설계

### 4.1 역할 모델

역할은 `PlayerState`에 replicated property로 둔다.

초기 설계상 후보:

- `ERelayOperatorRole`
- `ARelayPlayerState`
- `AIrisDemoGameMode`
- `AIrisDemoPlayerController`

현재 구현 class:

- `ERelayOperatorRole`: Scenario A 플레이어 역할 enum
- `ARelayPlayerState`: replicated `OperatorRole`, `AssignedZoneId` 보관
- `AIrisDemoGameMode`: `PostLogin`에서 접속 순서 기반 role 자동 할당, FieldAgent zone 자동 할당
- `AIrisDemoPlayerController`: local controller의 role/zone 확인 로그 출력

역할 할당 방식은 초기에는 간단하게 시작한다.

1. 서버가 접속 순서에 따라 역할을 부여한다.
2. 개발용 console command 또는 UI로 역할을 변경한다.
3. 역할 변경 시 서버가 filtering 대상 갱신을 요청한다.

나중에 Lobby가 생기면 역할 선택 UI로 교체한다.

### 4.2 테스트 actor 모델

Scenario A actor는 gameplay보다 replication 정책 검증에 집중한다.

공통 속성:

- `SensorId`/`DroneId`/`CrateId` 같은 replicated stable id
- `ZoneId`
- `InterestCategory`
- `InterestDetailLevel`
- `DebugName`
- `LastUpdateSequence`
- `bScenarioAEnabled`

actor별 replicated 속성:

- Sensor: `AlertLevel`, `bTriggered`
- Drone: `BatteryPercent`, `EDroneRelayState`
- SupplyCrate: `StockCount`, `bReserved`
- Summary: `KnownAlertCount`, `KnownDroneCount`, `KnownSupplyCount`

서버는 일정 주기 또는 입력으로 sequence 값을 올리고, 클라이언트는 자신에게 도착한 마지막 sequence를 표시한다.

#### 여러 actor 종류를 두는 이유

`ARelaySensorActor` 하나만으로도 replicated property와 `OnRep` 동작 자체는 확인할 수 있다. 따라서 Sensor/Drone/SupplyCrate/Summary를 나누는 목적은 단순 복제 확인이 아니라, 이후 role-based filtering에서 서로 다른 category의 정보를 다르게 다루기 위한 것이다.

Scenario A의 핵심 판단식은 “이 actor가 복제되는가?”가 아니라 “이 connection role이 이 category/zone/detail 수준의 정보를 받아야 하는가?”이다.

```text
PlayerRole + ActorCategory + ZoneId + DetailLevel
-> FilteringPolicy가 해당 connection에 actor를 허용할지 결정
```

각 actor 종류는 이후 filtering policy의 서로 다른 판단 축을 대표한다.

| Actor | 대표 category | 보여줄 정책 차이 |
|-------|---------------|------------------|
| `ARelaySensorActor` | `SensorDetail` | zone별 정적 경보/detail 정보. FieldAgent는 자기 zone 중심, Commander는 전체 또는 요약, Spectator는 상세 제한 |
| `ARelayDroneActor` | `DroneDetail` | 이동 작전 자산/detail 정보. FieldAgent는 자기 zone 또는 근처 drone, Spectator는 detail 차단, Commander는 전체 자산 상태 또는 summary |
| `RelaySupplyCrateActor` | `SupplyDetail` | 보급/소유/예약 상태. 이후 owner/squad/zone 조건을 섞기 위한 대상 |
| `ARelayOperationalSummaryActor` | `OperationalSummary` | Commander/Spectator가 받을 축약 정보. detail actor를 받지 않아도 전체 상황을 설명할 수 있게 함 |

현재 A4 구현은 actor별 custom filter class를 새로 만들지 않고, UE 5.7.1의 `UReplicationSystem` group filtering API를 사용한다. 상세 actor는 zone별 exclusion group에 넣고, summary actor는 별도 summary group에 넣는다. 각 connection의 `ARelayPlayerState::OperatorRole`과 `AssignedZoneId`를 기준으로 group status를 `Allow` 또는 `Disallow`로 갱신한다.

`ScenarioAEnableRoleFiltering=1` 옵션을 켤 때만 filtering을 적용한다. 이 옵션을 끄면 A3 baseline처럼 role과 무관하게 detail actor total을 비교할 수 있다.

Scenario A의 role filtering 기본값은 `Config/DefaultGame.ini`의 GameMode 섹션에서 설정할 수 있다. `InitGame`의 `Options` 인자는 `?Name=Player` 같은 URL option만 담고, 프로세스 command line 전체를 담지 않는다. Scenario A 옵션 적용 코드는 `DefaultGame.ini` 값을 먼저 읽고, URL option 또는 `FCommandLine::Get()`에서 읽은 command line option이 실제로 전달되면 그 값을 최종값으로 덮어쓴다.

프로젝트의 replication mode 기본값은 `Config/DefaultEngine.ini`의 `net.Iris.UseIrisReplication=1`이다. 로컬 실행 배치도 기본적으로 `-UseIrisReplication=1 -net.Iris.UseIrisReplication=1`을 전달한다. Generic baseline은 프로젝트 기본값이 아니라 기존 replication path와 비교하기 위한 control run이므로 `--no-iris` 또는 `-UseIrisReplication=0`으로 명시 실행한다.

```ini
[/Script/IrisDemo.IrisDemoGameMode]
bScenarioAEnableRoleFiltering=True
```

옵션 적용 여부는 서버 로그의 `Scenario A options applied`에서 확인한다. `UrlOptions`는 `InitGame`에서 받은 URL option이고, `CommandLine`은 프로세스 command line이다. `Sources=[...]` 항목이 `CommandLine`이면 `-ScenarioA...` 인자가 적용된 것이고, `RoleFilteringSource=UrlOptions` 또는 `RoleFilteringSource=CommandLine`이면 `?ScenarioAEnableRoleFiltering=1` 또는 `-ScenarioAEnableRoleFiltering=1`이 ini 값을 덮어쓴 것이다.

### 4.3 Filtering 정책

정책은 처음부터 엔진 내부를 과하게 추상화하지 않고, 다음 두 단계로 나눈다.

1. Baseline relevancy
   - 기존 actor replication/relevancy로 역할별 차이를 최소한 재현한다.
   - owner-only, distance relevancy, dormancy 사용 가능성을 확인한다.

2. Iris filtering
   - UE 5.7.1에서 사용 가능한 공식 Iris filtering API를 기준으로 connection/group filter 적용 지점을 찾는다.
   - role/zone을 group 또는 connection 조건으로 매핑한다.
   - Replication Graph는 도입하지 않는다.

현재 A4 구현 정책:

| 역할 | Detail group | Summary group |
|------|--------------|---------------|
| Commander | 모든 zone detail 허용 | 허용 |
| FieldAgent | `AssignedZoneId`와 같은 zone detail만 허용 | 제한 |
| Spectator | 모든 detail 제한 | 허용 |

현재는 FieldAgent 기준을 zone으로만 잡고, distance 기반 확장은 다음 단계로 남긴다. Spectator는 summary actor만 받도록 제한하지만, summary update cadence를 따로 늦추는 delayed summary 처리는 아직 별도 TODO다.

구현 중 새로 확인한 API 제약은 이 문서의 “결정 로그”와 `Docs/Iris기본지식.md`에 반영한다.

### 4.4 측정과 표시

최소 측정값:

- connection role
- server-side test actor total count
- client-side received test actor count
- role별 last received sequence
- Generic/Iris mode
- 실행 seed

가능하면 추가할 값:

- relevant object count per connection
- replicated object count
- outgoing bandwidth
- packet size

초기에는 log/console 출력으로 시작하고, 이후 UMG debug widget으로 승격한다.

## 5. 단계별 체크리스트

체크리스트는 구현이 끝날 때마다 같은 PR/커밋에서 갱신한다. 완료 표시만 하지 말고, 검증 근거가 있으면 “검증 메모”에 날짜와 명령/결과를 남긴다.

### A0. 기반 정리

- [x] UE 5.7.1 기준으로 프로젝트명/모듈명을 `IrisDemo`로 유지
- [x] Iris plugin 활성화
- [x] `SetupIrisSupport(Target)` 유지
- [x] `TargetRules.bUseIris` 미사용 확인
- [x] `DefaultEngine.ini` Iris 기본 설정 확인
- [x] Scenario A 문서가 `planning.md`에서 링크됨
- [x] Scenario A 첫 구현 범위를 커밋 단위로 확정

검증 메모:

- 2026-05-15: `.uproject`, `IrisDemo.Build.cs`, `Target.cs`, `DefaultEngine.ini` 수동 확인
- 2026-05-15: `planning.md`에서 Scenario A 상세 문서 링크 추가, 다음 커밋 후보를 역할 골격으로 확정

### A1. 역할 골격

- [x] `ERelayOperatorRole` 정의
- [x] 역할을 저장할 `PlayerState` 또는 기존 PlayerState 확장 방향 결정
- [x] 서버 권위 역할 할당 함수 작성
- [x] 클라이언트에서 현재 역할 확인 가능
- [x] 역할 변경 시 로그 출력
- [x] 2 client 이상 PIE에서 역할이 다르게 보이는지 확인

완료 기준:

- Commander/FieldAgent/Spectator 세 역할을 서버가 구분하고, 각 클라이언트가 자기 역할을 표시할 수 있다.

검증 메모:

- 2026-05-15: `ARelayPlayerState`와 접속 순서 기반 `PostLogin` 역할 할당 구현
- 2026-05-15: UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공
- 2026-05-15: 사용자 제공 PIE 로그에서 Commander/FieldAgent role replicated 및 local ready 로그 확인

### A2. Scenario A 테스트 actor

- [x] `RelaySensorActor` C++ class 작성
- [x] `RelayDroneActor` C++ class 작성
- [x] `RelaySupplyCrateActor` C++ class 작성
- [x] summary actor 또는 GameState summary 작성
- [x] 서버에서 테스트 actor를 deterministic하게 spawn
- [x] actor별 replicated property와 `OnRep` 로그 작성
- [x] 로그/측정용 replicated stable id 작성
- [x] filtering policy 입력용 공통 interest metadata 작성
- [x] `RelaySensorActor`를 월드에서 볼 수 있도록 기본 visual mesh 추가
- [ ] actor count/zone 배치 seed를 설정으로 분리

완료 기준:

- 서버 1개, 클라이언트 2개 이상에서 actor 상태 변경 sequence가 클라이언트에 복제된다.

검증 메모:

- 2026-05-15: `ARelaySensorActor` 추가. `ZoneId`, `DebugName`, `LastUpdateSequence`, `bScenarioAEnabled`, `AlertLevel`, `bTriggered` 복제와 `OnRep` 로그 작성.
- 2026-05-15: UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공. PIE 복제 확인 예정.
- 2026-05-15: `AIrisDemoGameMode`가 서버 BeginPlay에서 `ARelaySensorActor` 6개를 zone별 deterministic 위치에 spawn하고 2초마다 상태를 갱신하도록 구현. PIE에서 `Scenario A sensors spawned`, `Relay sensor updated`, `Relay sensor replicated` 로그 확인 예정.
- 2026-05-15: UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.
- 2026-05-15: 서버/클라이언트의 local `Actor=RelaySensorActor_N` 이름이 서로 다를 수 있음을 PIE 로그로 확인. `ARelaySensorActor`에 replicated `SensorId`를 추가하고 로그 식별 기준을 `SensorId`, `DebugName`, `ZoneId`, `Sequence`로 변경.
- 2026-05-15: 데모 가시성을 위해 `RelaySensorActor`에 visual mesh/component를 추가하기로 결정. 진행 시점은 `SensorId` 로그 확인 후, `RelayDroneActor`/`RelaySupplyCrateActor` 확장 전으로 둔다.
- 2026-05-15: `ARelaySensorActor`에 기본 cylinder mesh와 text label을 추가. label은 `DebugName`, `SensorId`, `ZoneId`, `AlertLevel`을 표시하고 trigger 상태에서 빨간색으로 바뀐다. UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.
- 2026-05-15: `ARelayDroneActor` 추가. `ZoneId`, `DebugName`, `LastUpdateSequence`, `bScenarioAEnabled`, `BatteryPercent`, `DroneState` 복제와 `OnRep` 로그 작성. `AIrisDemoGameMode`가 drone 3개를 zone별 deterministic 위치에 spawn하고 sensor update timer에서 함께 상태를 갱신하도록 구현. UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.
- 2026-05-15: `ERelayInterestCategory`, `ERelayInterestDetailLevel` 추가. `ARelaySensorActor`와 `ARelayDroneActor`가 category/detail metadata를 복제하고 로그에 함께 출력하도록 구현. UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.
- 2026-05-15: `ARelaySupplyCrateActor` 추가. `ZoneId`, `OwningSquadId`, `InterestCategory=SupplyDetail`, `InterestDetailLevel=Detail`, `StockCount`, `bReserved` 복제와 `OnRep` 로그 작성. 보급품은 이후 role/category/zone뿐 아니라 squad/owner 계열 filtering 조건을 검증하기 위한 detail actor로 사용한다. UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.
- 2026-05-15: `ARelayOperationalSummaryActor` 추가. `InterestCategory=OperationalSummary`, `InterestDetailLevel=Summary`, `KnownAlertCount`, `KnownDroneCount`, `KnownSupplyCount` 복제와 `OnRep` 로그 작성. `AIrisDemoGameMode`가 summary actor 1개를 spawn하고 sensor/drone/supply 상태 변경에 맞춰 summary count를 갱신한다. UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.

PIE 확인 방법:

1. ThirdPerson 맵에서 listen server + 2 clients PIE 실행.
2. Output Log에서 `Scenario A sensors spawned: Count=6`, `Scenario A drones spawned: Count=3`, `Scenario A supply crates spawned: Count=3`, `Scenario A operational summary spawned: Count=1` 확인.
3. 서버 로그에서 `Relay sensor updated`, `Relay drone updated`, `Relay supply crate updated`가 2초마다 증가하는지 확인.
4. 클라이언트 로그에서 `Relay sensor replicated`, `Relay drone replicated`, `Relay supply crate replicated`와 `Sequence` 증가를 확인.
5. 서버/클라이언트의 local actor 이름은 서로 다를 수 있으므로, 로그 비교 식별자는 replicated stable id, `DebugName`, `ZoneId`, `Category`, `DetailLevel`, `Sequence`를 사용한다.

### A3. Baseline 비교 기준

A3는 A4의 role-based filtering을 넣기 전 control run이다. 이 단계의 기대값은 역할별 차이가 나는 것이 아니라, Generic/Iris 양쪽에서 같은 seed/count/duration 조건으로 모든 상세 actor가 동일하게 수신되는지 확인하는 것이다. A4 이후에는 이 표가 "filtering 전 기준선"이 된다.

고정 실행 조건:

- 서버 형태: 먼저 listen server + 2 clients PIE로 확인하고, 이후 dedicated server + 2 clients로 반복한다.
- 기본 인원: 2 clients에서는 Commander/FieldAgent만 기록하고, Spectator 비교는 3 clients에서 기록한다.
- 기본 actor count: sensor 60, drone 30, supply crate 30, detail actor total 120.
- 기본 seed/duration: `ScenarioASeed=1001`, `ScenarioARunDuration=95`, `ScenarioAUpdateInterval=2`.
- 기본 bandwidth 측정 window: `ScenarioANetworkMetricsStartDelay=35`, `ScenarioANetworkMetricsDuration=60`, `ScenarioANetworkMetricsInterval=1`.
- A4 이전 기대값: Commander/FieldAgent/Spectator 모두 `DetailActorTotal=120`이 기준이다. 역할별 차이가 이미 난다면 filtering 결과가 아니라 baseline 복제 조건 문제로 본다.

실행 인자:

- Generic replication: `--no-iris` 또는 `-UseIrisReplication=0`
- Iris replication: 기본값 또는 `-UseIrisReplication=1`
- 공통 조건은 코드 기본값을 사용한다. 로컬 실행 배치는 `--no-iris`일 때 `-ScenarioAEnableRoleFiltering=0`, `--iris`일 때 `-ScenarioAEnableRoleFiltering=1`을 자동 전달한다.
- PIE URL option으로 넘길 때는 같은 이름을 `?ScenarioASeed=1001?ScenarioASensorCount=60?...` 형식으로 붙인다.

기록할 로그:

- 서버 시작 기준: `Scenario A baseline config`
- 서버 측정 구간 종료: `Scenario A baseline server window complete`
- 서버 bandwidth 측정 window 시작/종료: `Scenario A network metrics window started`, `Scenario A network metrics window stopped`
- 각 local client 수신 기준: 기본값은 95초 뒤 자동 기록이고, 필요할 때 console에서 `IrisRelayLogBaselineSnapshot`을 실행해 수동 기록

자동 파일 기록:

- 서버 `GameMode`는 BeginPlay 이후 `Saved/ScenarioA/Runs/<RunId>/run.json`을 저장한다.
- 서버 `GameMode`는 `ScenarioANetworkMetricsInterval`이 0보다 크면 같은 RunId의 `server_network_metrics.csv`에 NetDriver 전체 및 connection별 outgoing bandwidth row를 주기적으로 append한다. `ScenarioANetworkMetricsStartDelay`가 0보다 크면 지정된 지연 후 측정을 시작하고, `ScenarioANetworkMetricsDuration`이 0보다 크면 해당 길이만큼만 기록한다.
- 각 local client는 `-ScenarioAAutoSnapshotDelay=<seconds>`가 0보다 크면 같은 RunId의 `client_snapshots.csv`에 `Source=Auto` row를 한 번 append한다. `IrisRelayLogBaselineSnapshot`을 수동 실행하면 같은 파일에 `Source=Console` row를 추가 append한다.
- `RunId`는 `-ScenarioARunId=<id>`로 넘기며, `Tools/RunLocalMultiplay.bat`와 `Tools/RunLocalBinariesMultiplay.bat`는 기본적으로 `ScenarioA_yyyyMMdd_HHmmss` 형식의 RunId를 서버와 모든 client에 공통 전달한다.
- 수동으로 고정하려면 배치 실행 시 `--run-id ScenarioA_manual_001`처럼 넘긴다.
- 수집 완료 후 창을 직접 닫지 않으려면 local binary 배치에 `--auto-close`를 붙인다. 이 옵션은 server/client에 `-ScenarioAAutoExit=1 -ScenarioAAutoExitGraceSeconds=5`를 전달한다. 서버는 run duration과 network metrics window가 끝난 뒤 Grace 이후 종료하고, client는 auto snapshot 기록 뒤 Grace 이후 종료한다.

`run.json` 주요 필드:

- `runId`, `timestamp`, `map`, `serverType`, `mode`
- `irisCommandLineOverride`, `roleFiltering`, `roleFilteringSource`, `runIdSource`
- `seed`, `sensorCount`, `droneCount`, `supplyCrateCount`, `summaryCount`, `detailActorTotal`
- `updateInterval`, `runDuration`, `networkMetricsInterval`, `networkMetricsStartDelay`, `networkMetricsDuration`, `commandLine`

`server_network_metrics.csv` 컬럼:

```csv
Timestamp,RunId,Mode,NetMode,Scope,ConnectionId,Controller,Role,Zone,
OutBytesPerSecond,OutTotalBytes,OutPacketsPerSecond,OutTotalPackets,
OutBunches,OutTotalBunches,ConnectionCount,WorldTimeSeconds,MetricsWindowElapsedSeconds
```

- `Scope=NetDriver` row는 서버 NetDriver 전체 outgoing bytes/sec와 누적 송신량을 기록한다.
- `Scope=Connection` row는 각 client connection별 outgoing bytes/sec와 누적 송신량을 기록한다.

`client_snapshots.csv` 컬럼:

```csv
Timestamp,RunId,Source,Mode,NetMode,Role,Zone,Controller,
SensorCount,DroneCount,SupplyCrateCount,SummaryCount,DetailActorTotal,
SensorZ0,SensorZ1,SensorZ2,SensorUnknown,
DroneZ0,DroneZ1,DroneZ2,DroneUnknown,
SupplyCrateZ0,SupplyCrateZ1,SupplyCrateZ2,SupplyCrateUnknown,
SensorLastSequence,DroneLastSequence,SupplyCrateLastSequence,SummaryLastSequence,DetailMaxSequence
```

- [x] Generic replication 실행 인자 정리: `-UseIrisReplication=0`
- [x] Iris 실행 인자 정리: `-UseIrisReplication=1`
- [x] 같은 seed/count/duration으로 실행하는 절차 작성
- [x] Generic mode에서 역할별 수신 actor count 기록
- [x] Iris mode에서 역할별 수신 actor count 기록
- [x] 비교 결과를 `Docs/ScenarioA_RoleBasedFiltering.md`에 기록

완료 기준:

- Generic/Iris 실행 조건을 동일하게 맞춘 비교 표가 최소 1회 기록된다.

검증 메모:

- 2026-05-15: baseline 비교용 실행 옵션을 `AIrisDemoGameMode`에서 읽도록 추가. `ScenarioASeed`, actor count, update interval, run duration을 command line 또는 PIE URL option으로 고정할 수 있다.
- 2026-05-15: `IrisRelayLogBaselineSnapshot` console command 추가. 각 local client에서 현재 role, Generic/Iris mode, sensor/drone/supply crate 수신 수, detail actor total을 로그로 남긴다.
- 2026-05-15: UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.
- 2026-05-15: A4 구현 이후에도 `ScenarioAEnableRoleFiltering=0`을 명시하면 A3 baseline control run을 이어갈 수 있도록 유지했다. snapshot 로그에는 role zone과 summary count가 추가되었다.
- 2026-05-19: 사용자 제공 Commander snapshot 기준으로 Generic/Iris 모두 `SensorCount=6`, `DroneCount=3`, `SupplyCrateCount=3`, `SummaryCount=1`, `DetailActorTotal=12`를 확인했다. Generic은 `DetailMax=11`, Iris는 `DetailMax=4`로 측정 시점은 다르지만 actor count와 zone 분포는 동일했다. FieldAgent/Spectator snapshot은 아직 미측정이다. `Tools\BuildEditor.bat`로 UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.
- 2026-05-20: 사용자 제공 Generic snapshot 기준으로 Commander/FieldAgent/Spectator 모두 `DetailActorTotal=12`, `SummaryCount=1`을 수신했다. role과 무관하게 `SensorZones=2/2/2`, `DroneZones=1/1/1`, `SupplyCrateZones=1/1/1`이므로 Generic baseline 기대값과 일치한다.
- 2026-05-22: `Docs/학습/결과임시기록.md`의 local binary snapshot 기준으로 listen server와 dedicated server 모두 Generic baseline에서 Commander/FieldAgent/Spectator가 `DetailActorTotal=12`, `SummaryCount=1`을 수신했다. 각 role의 zone 분포도 `SensorZones=2/2/2`, `DroneZones=1/1/1`, `SupplyCrateZones=1/1/1`로 동일해 A3 control run 기대값과 일치한다.
- 2026-05-22: `-ScenarioARunId` 기반 run 폴더를 추가했다. 서버는 `run.json`에 실행 조건을 저장하고, `IrisRelayLogBaselineSnapshot`은 `client_snapshots.csv`에 local client 수신 결과를 append한다. `Tools/RunLocalMultiplay.bat`와 `Tools/RunLocalBinariesMultiplay.bat`는 같은 RunId를 서버와 client에 공통 전달한다.
- 2026-05-28: 서버가 `server_network_metrics.csv`에 NetDriver 전체와 connection별 `OutBytesPerSecond`, `OutTotalBytes`를 append하도록 추가했다. 첫 측정값 수집 경로는 engine `UNetDriver`/`UNetConnection` 통계를 사용한다.

### A4. Role-Based Filtering 구현

- [x] UE 5.7.1 공식 문서/헤더 기준으로 Iris filtering 적용 API 확인
- [x] Commander connection에 전체 summary/detail 허용
- [x] FieldAgent connection에 zone/distance 기반 detail 허용
- [x] Spectator connection에 summary만 허용
- [ ] Spectator summary update cadence 지연 처리
- [x] `DefaultGame.ini` 기반 role filtering opt-in 지원
- [ ] 역할 변경 시 filter membership 갱신
- [x] actor spawn 시 filter membership 갱신
- [ ] actor despawn 시 filter membership 정리
- [x] filtering 실패 시 fallback 로그 또는 ensure 추가

완료 기준:

- 같은 서버 상태에서 역할별 클라이언트가 수신하는 detailed actor set이 다르고, 의도하지 않은 상세 actor가 Spectator에 복제되지 않는다.

검증 메모:

- 2026-05-15: UE 5.7.1 헤더 기준으로 `UReplicationSystem::CreateGroup`, `AddExclusionFilterGroup`, `AddToGroup`, `SetGroupFilterStatus`, `UObjectReplicationBridge::GetReplicatedRefHandle` 사용 가능 확인.
- 2026-05-15: `ScenarioAEnableRoleFiltering=1`일 때 detail actor를 zone별 exclusion group에 넣고, summary actor를 `ScenarioA_OperationalSummary` group에 넣도록 구현. connection id는 `APlayerController::GetNetConnection()->GetConnectionHandle().GetParentConnectionId()`를 사용한다.
- 2026-05-15: `Tools\BuildEditor.bat`로 UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공. PIE role별 actual count 검증은 아직 남아 있다.
- 2026-05-20: 사용자 제공 Iris snapshot 기준으로 Commander는 `DetailActorTotal=12`, `SummaryCount=1`을 수신했고, FieldAgent(Zone 0)는 `DetailActorTotal=4`, `SummaryCount=0`, Spectator는 `DetailActorTotal=0`, `SummaryCount=1`을 수신했다. FieldAgent는 Zone 0 detail만 받고 Spectator는 detail actor를 받지 않아 현재 group filtering 정책과 일치한다. `Tools\BuildEditor.bat`로 UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.
- 2026-05-22: `Docs/학습/결과임시기록.md`의 listen server와 dedicated server Iris snapshot 모두에서 Commander는 `DetailActorTotal=12`, `SummaryCount=1`, FieldAgent(Zone 0)는 `DetailActorTotal=4`, `SummaryCount=0`, Spectator는 `DetailActorTotal=0`, `SummaryCount=1`을 수신했다. FieldAgent는 Zone 0의 `Sensor=2`, `Drone=1`, `SupplyCrate=1`만 받았고, Spectator는 detail actor를 받지 않아 두 서버 형태에서 A4 group filtering 정책이 같은 결과로 재현되었다.

실행 인자:

- A4 Iris filtering: 로컬 배치 기준 `Tools\RunLocalBinariesMultiplay.bat --iris --run-id ScenarioA_IrisHeavy_001`
- Generic mode 또는 `ScenarioAEnableRoleFiltering=0`에서는 role filtering을 적용하지 않는다.
- Generic mode는 IrisDemo의 기본 실행 경로가 아니라 비교용 control run이다. 로컬 배치에서는 `--no-iris`를 사용하고, 직접 실행할 때는 `-UseIrisReplication=0`을 명시한다.
- PIE URL option이 `AIrisDemoGameMode::InitGame`의 `Options`로 전달되지 않는 경우, `Config/DefaultGame.ini`에서 `bScenarioAEnableRoleFiltering=True`로 켠 뒤 에디터를 다시 실행한다.
- 적용 확인 로그: `Scenario A options applied: ... RoleFiltering=Enabled RoleFilteringSource=DefaultGame.ini`

### A5. Debug UI/로그

- [x] 현재 role 표시
- [x] local received sensor/drone/crate count 표시
- [x] zone별 received count 표시
- [x] last sequence 표시
- [x] Generic/Iris mode 표시
- [x] server summary log 출력
- [x] 결과 캡처용 console command 또는 key binding 작성

완료 기준:

- 에디터 PIE 화면 또는 로그만 보고도 역할별 관심 필터링 결과를 설명할 수 있다.

검증 메모:

- 2026-05-19: `IrisRelayLogBaselineSnapshot` 로그에 `SensorZones`, `DroneZones`, `SupplyCrateZones`와 `LastSequences`를 추가했다. 각 local client에서 실제 수신한 detail actor의 zone 분포와 마지막 수신 sequence를 한 줄로 기록하므로 A3/A4 PIE 결과 표 작성에 바로 사용할 수 있다. `Tools\BuildEditor.bat`로 UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.

### A6. 검증

- [x] UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공
- [ ] listen server + 2 clients PIE 검증
- [x] listen server + 3 clients local binary 실행 검증
- [x] dedicated server + 3 clients local binary 실행 검증
- [x] Generic/Iris 양쪽에서 실행 성공
- [x] 역할별 expected/actual actor count 표 작성
- [x] false positive 복제 목록 확인
- [x] bandwidth 또는 replicated object count 수집 가능성 확인

완료 기준:

- Scenario A를 2분 안에 재현할 수 있고, 결과를 문서의 표로 설명할 수 있다.

### A6.5. 성능/디버깅 포트폴리오 보강

- [x] dedicated server + 3 clients에서 Generic/Iris 결과 재현
- [x] PIE URL option, command line option, `DefaultGame.ini` override 적용 차이 기록
- [x] run id, seed, mode, role filtering, actor count, duration을 파일로 저장
- [x] actor count 외에 outgoing bandwidth 또는 replicated object count 중 최소 1개 수집
- [x] 120 actor heavy 조건에서 Generic/Iris bandwidth 비교 표 작성
- [x] 무효 run 원인 분석과 유효 run 판정 기준을 `DebuggingNotes.md`에 기록
- [x] Unreal Insights / CSV / NetTrace 중 최소 1개 수집 경로 작성
- [ ] Commander bandwidth 감소 원인을 반복 run 또는 NetTrace/CSV profiling으로 추가 확인

완료 기준:

- Scenario A가 “Iris filtering 기능 확인”을 넘어, 실행 조건과 측정 결과를 재현 가능한 형태로 설명할 수 있다.

### A7. 마무리 문서화

- [ ] 구현 class 목록 추가
- [ ] 실행 방법 추가
- [x] 측정 결과 표 추가
- [x] 디버깅 노트 추가
- [x] 비교 실험 기본기 문서 추가
- [ ] UE 5.7.1 Iris filtering 제약/주의점 추가
- [ ] 다음 시나리오로 넘길 TODO 분리
- [ ] `Docs/Iris기본지식.md`에 새로 확인한 Iris 사실 반영

완료 기준:

- 제3자가 문서만 보고 Scenario A의 목적, 실행법, 결과 해석, 남은 한계를 이해할 수 있다.

## 6. 결과 기록 표

| 날짜 | 모드 | 서버 형태 | 클라이언트 수 | Seed | Commander 수신 | FieldAgent 수신 | Spectator 수신 | 메모 |
|------|------|-----------|---------------|------|-----------------|------------------|-----------------|------|
| 2026-05-19~20 | Generic | PIE, 형태 미기록 | 3 role snapshot | 1001 | Detail=12, Summary=1, Seq=11 | Detail=12, Summary=1, Seq=5 | Detail=12, Summary=1, Seq=7 | A3 baseline. 모든 role이 detail 12개와 summary 1개 수신 |
| 2026-05-19~20 | Iris | PIE, 형태 미기록 | 3 role snapshot | 1001 | Detail=12, Summary=1, Seq=4 | Detail=4, Summary=0, Seq=6 | Detail=0, Summary=1, Seq=29 | A4 filtering. FieldAgent는 Zone 0 detail만, Spectator는 summary만 수신 |
| 2026-05-22 | Generic | Dedicated server, local binary | 3 role snapshot | 1001 | Detail=12, Summary=1, DetailMax=27 | Detail=12, Summary=1, DetailMax=18 | Detail=12, Summary=1, DetailMax=32 | A3 baseline 재현. 모든 role이 전체 detail과 summary 수신 |
| 2026-05-22 | Iris | Dedicated server, local binary | 3 role snapshot | 1001 | Detail=12, Summary=1, DetailMax=11 | Detail=4, Summary=0, DetailMax=18 | Detail=0, Summary=1, DetailMax=0 | A4 filtering 재현. FieldAgent는 Zone 0 detail만, Spectator는 summary만 수신 |
| 2026-05-22 | Generic | Listen server, local binary | 3 role snapshot | 1001 | Detail=12, Summary=1, DetailMax=18 | Detail=12, Summary=1, DetailMax=13 | Detail=12, Summary=1, DetailMax=15 | A3 baseline 재현. 서버 형태가 바뀌어도 role별 차이 없음 |
| 2026-05-22 | Iris | Listen server, local binary | 3 role snapshot | 1001 | Detail=12, Summary=1, DetailMax=9 | Detail=4, Summary=0, DetailMax=7 | Detail=0, Summary=1, DetailMax=0 | A4 filtering 재현. Dedicated server 결과와 같은 actor count 패턴 |
| 2026-05-30 | Generic heavy | Dedicated server, local binary | 3 clients | 1001 | Detail=120, Summary=1, AvgOut=2280.28 | Detail=120, Summary=1, AvgOut=2307.00 | Detail=120, Summary=1, AvgOut=2263.84 | `ScenarioA_GenericHeavy_002`. 60초 metrics window, NetDriver DeltaOutTotalBytes=411758 |
| 2026-05-30 | Iris heavy | Dedicated server, local binary | 3 clients | 1001 | Detail=120, Summary=1, AvgOut=2093.87 | Detail=40, Summary=0, AvgOut=1740.18 | Detail=0, Summary=1, AvgOut=1571.51 | `ScenarioA_IrisHeavy_002`. FieldAgent -24.6%, Spectator -30.6%, NetDriver DeltaOutTotalBytes=325034 |
| 2026-06-01 | Generic heavy repeat | Dedicated server, local binary | 3 clients | 1001 | Detail=120, Summary=1, AvgOut=2338.31 | Detail=120, Summary=1, AvgOut=2338.30 | Detail=120, Summary=1, AvgOut=2337.30 | `ScenarioA_20260601_201201`. 60초 metrics window, NetDriver DeltaOutTotalBytes=419329 |
| 2026-06-01 | Iris heavy repeat | Dedicated server, local binary | 3 clients | 1001 | Detail=120, Summary=1, AvgOut=2109.62 | Detail=40, Summary=0, AvgOut=1755.75 | Detail=0, Summary=1, AvgOut=1579.67 | `ScenarioA_20260601_201615`. FieldAgent -24.9%, Spectator -32.4%, NetDriver DeltaOutTotalBytes=327108 |
| 2026-06-01 | Generic heavy final repeat | Dedicated server, local binary | 3 clients | 1001 | Detail=120, Summary=1, AvgOut=2334.49 | Detail=120, Summary=1, AvgOut=2333.87 | Detail=120, Summary=1, AvgOut=2334.34 | `ScenarioA_20260601_203950`. 60초 metrics window, NetDriver DeltaOutTotalBytes=419448. 서버가 남은 client 하나보다 먼저 종료된 정황은 있으나 snapshot/metrics는 유효 |
| 2026-06-01 | Iris heavy final repeat | Dedicated server, local binary | 3 clients | 1001 | Detail=120, Summary=1, AvgOut=2097.70 | Detail=40, Summary=0, AvgOut=1761.10 | Detail=0, Summary=1, AvgOut=1577.85 | `ScenarioA_20260601_204211`. FieldAgent -24.5%, Spectator -32.4%, NetDriver DeltaOutTotalBytes=326707 |

## 7. 결정 로그

| 날짜 | 결정 | 근거 | 후속 작업 |
|------|------|------|-----------|
| 2026-05-15 | Scenario A는 별도 문서에서 체크리스트를 관리한다. | `planning.md`는 전체 기획 문서라 구현 진행 체크를 계속 누적하기에 길어질 수 있음 | 구현이 진행될 때 이 문서를 함께 갱신 |
| 2026-05-15 | 첫 구현 범위에서 UObject/subobject, prioritization, seamless travel은 제외한다. | Scenario A의 핵심은 role/connection별 filtering 결과를 먼저 증명하는 것 | 이후 Scenario C/B/D에서 확장 |
| 2026-05-15 | A3 baseline은 A4 filtering 전 control run으로 고정한다. | 아직 role-based filtering을 넣기 전이므로 역할별 actor count는 같아야 Generic/Iris 비교 기준으로 쓸 수 있음 | Generic/Iris를 같은 seed/count/duration으로 각각 실행하고 실제 수신 수를 결과 표에 기록 |
| 2026-05-15 | A4 filtering은 `ScenarioAEnableRoleFiltering=1` opt-in으로 둔다. | A3 baseline 실제 측정이 아직 비어 있으므로 필터를 기본 적용하면 pre-filter control run을 잃게 됨 | A3는 옵션 off, A4는 옵션 on으로 각각 결과 표를 기록 |
| 2026-05-15 | 첫 A4 구현은 Iris group filtering으로 시작한다. | UE 5.7.1 공개 헤더에서 group 생성/멤버십/connection별 status API가 확인되며, custom `UNetObjectFilter`보다 작은 빌드 단위로 검증 가능 | PIE에서 role별 actual count 확인 후 distance/role-change 갱신으로 확장 |
| 2026-05-15 | A4 role filtering opt-in은 `DefaultGame.ini`에서도 설정할 수 있게 한다. | Rider/PIE 설정의 URL option이 `AIrisDemoGameMode::InitGame` `Options`로 전달되지 않는 경우가 있어, 코드 수정 없이 재현 가능한 설정 경로가 필요함 | 서버 로그의 `Scenario A options applied`와 `RoleFilteringSource`로 실제 적용 경로 확인 |
| 2026-05-22 | Scenario A의 Iris 판정은 CVar 단독 대신 실제 `NetDriver->IsUsingIrisReplication()`을 우선 사용한다. | UE 5.7.1은 `-UseIrisReplication=1` command line으로 NetDriver를 Iris로 강제할 수 있지만, 이후 config load로 `net.Iris.UseIrisReplication` CVar 로그가 0으로 보일 수 있음 | `Scenario A options applied`, `Scenario A baseline config`, `Scenario A role filtering applied/skipped` 로그의 `IrisMode`/`Mode`로 확인 |
| 2026-05-26 | IrisDemo의 기본 replication mode는 Iris로 둔다. | 이 프로젝트는 Iris 데모이므로 `net.Iris.UseIrisReplication=1`이 자연스러운 기본값이다. Generic은 기존 replication path와 비교하기 위한 control run이다. | local run batch 기본값은 Iris 유지, Generic 비교는 `--no-iris` 또는 `-UseIrisReplication=0`으로 명시 실행 |

## 8. 다음 작업 후보

가장 작은 다음 커밋 후보:

1. A7 마무리 문서화: 구현 class 목록, 실행 방법, UE 5.7.1 Iris 제약/주의점, 다음 시나리오 TODO를 정리
2. Unreal Insights trace run을 Generic/Iris 1쌍으로 실행하고 `server_trace.utrace`를 확인
3. Scenario C UObject/subobject replication 최소 구현 착수
4. 역할 변경 console command와 filter membership 갱신 검증 추가
5. actor despawn 시 filter membership 정리와 Spectator summary update cadence 지연 처리
6. `Docs/Iris기본지식.md`에 Scenario A에서 새로 확인한 실행/측정 제약 반영

detail actor category는 Sensor/Drone/Supply까지 채웠고, summary category와 A4 group filtering 결과도 listen server/dedicated server 양쪽 role별 snapshot으로 확인했다. 2026-06-01 기준으로 120 actor heavy 조건의 outgoing bandwidth 감소가 3쌍 측정에서 같은 방향으로 재현됐으므로, Scenario A의 actor filtering/bandwidth 측정은 여기서 마감한다. 다음부터는 UE profiling 산출물 수집 경로를 붙이거나 Scenario C UObject/subobject replication으로 확장한다.
