# Iris 데모 프로젝트 기획

## 1. 버전 선택: UE 5.7.1 기준

### 결론

**UE 5.7.1을 기준으로 개발**한다. 단, 문서 근거는 Epic의 UE 5.7 문서와 릴리즈 노트를 기준으로 삼고, 5.7.1 패치 노트에서 Iris 관련 별도 변경이 있는지는 실제 엔진 설치 후 한 번 더 확인한다.

5.6.1도 학습용으로는 가능하지만, 포트폴리오 제출용 데모라면 5.7 계열이 더 설득력 있다. 5.7 릴리즈 노트에서 Iris가 Experimental에서 Beta로 이동했고, Seamless Travel 지원과 Iris 내부 API/성능 개선이 명시되었기 때문이다.

### 5.6.1 vs 5.7.1 Iris 기능 비교

| 항목 | 5.6.1 | 5.7.1 기준 판단 |
|------|-------|-----------------|
| **기능 상태** | 공식 문서에서 Experimental 안내와 shipping 주의 문구 확인 | 5.7 릴리즈 노트에서 **Iris (Beta)**로 승격 확인 |
| **기본 복제 시스템** | 엔진 기본은 기존 Generic Replication이고 Iris는 opt-in | 프로젝트 기본값은 Iris로 명시 설정하고, Generic은 비교용 override로 실행 |
| **컴파일/빌드 설정** | Iris는 엔진에 기본 컴파일되지만 프로젝트에서 플러그인과 `SetupIrisSupport(Target)` 설정 필요 | 실제 UE 5.7.1 UBT에서 `TargetRules.bUseIris`는 존재하지 않음. Target.cs가 아니라 Build.cs/uproject/ini 중심으로 설정 |
| **런타임 활성화** | `net.Iris.UseIrisReplication=1` 또는 `-UseIrisReplication=1` | 동일한 방식으로 비교 실행 구성 가능 |
| **Seamless Travel** | 5.6 문서 기준으로는 5.7 릴리즈 노트의 추가 항목 전 상태 | 5.7 릴리즈 노트에서 Iris seamless travel support 명시 |
| **Subobject/UObject Replication** | Iris는 Registered Subobjects List 필요 | 5.7 문서 기준으로도 핵심 학습 항목. `RegisterReplicationFragments`까지 이해 필요 |
| **Replication Graph** | Iris와 Replication Graph는 별도 시스템 | 공식 migration 문서 기준, Iris는 Replication Graph를 지원하지 않으며 Iris Filtering/Prioritization이 대체 방향 |
| **Push Model** | Iris는 push 기반을 지향하며 미사용 시 polling fallback | `net.Iris.PushModelMode=1` 사용 시 `net.IsPushModelEnabled=1`도 함께 설정 |
| **BeginPlay/라이프사이클** | 공개 사례에서 BeginPlay 관련 주의점이 언급됨 | 5.7 릴리즈 노트는 `OnBeginReplication`, `EndReplication` 일관성 개선을 명시. BeginPlay 문제가 완전히 해결됐다고 쓰지는 않음 |

### 사실 확인 요약

- Epic 5.7 릴리즈 노트: Iris가 **Beta**로 이동했고, Actor Factory Overrides, `UReplicationBridge` 제거, `StartActorReplication`/`OnBeginReplication` 명명 정리, Iris/Legacy 간 `OnBeginReplication` 및 `EndReplication` 호출 일관성 개선, **Seamless Travel Support**, polling 최적화, parallel polling 관련 개선이 확인된다.
- Epic Iris 소개/설정 문서와 실제 UE 5.7.1 빌드 검증: Iris는 기존 replication system과 함께 존재하는 opt-in 시스템이다. 엔진은 Iris를 컴파일하지만 엔진 기본 복제 경로는 기존 시스템이며, IrisDemo는 포트폴리오 의도에 맞게 플러그인 활성화, `SetupIrisSupport(Target)`, `DefaultEngine.ini` 설정으로 Iris를 프로젝트 기본값으로 둔다. Generic은 같은 조건의 비교용 control run으로 명시 실행한다. 현재 설치된 UE 5.7.1에서는 `TargetRules.bUseIris`가 없어 Target.cs에 쓰면 빌드가 실패한다.
- Epic UObject 복제 문서: Iris는 virtual `ReplicateSubobjects` 방식을 지원하지 않고 Registered Subobjects List를 요구한다. 순수 `UObject`를 Iris로 복제하려면 `IsSupportedForNetworking`, `GetLifetimeReplicatedProps`, `AddReplicatedSubObject`, 필요 시 `RegisterReplicationFragments`를 이해해야 한다.
- Epic migration 문서: Replication Graph와 Iris는 동시에 쓰는 관계가 아니며, Iris Filtering/Prioritization이 Replication Graph의 역할을 대체하는 방향이다. 따라서 “Replication Graph Deprecated”라고만 적기보다 “Iris와 동시 사용 불가, 대체 개념은 Filtering/Prioritization”으로 설명한다.

### 참고 링크

- [Epic - Unreal Engine 5.7 Release Notes](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-7-release-notes?application_version=5.7)
- [Epic - Iris Replication System](https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-replication-system-in-unreal-engine?application_version=5.7)
- [Epic - Migrate to Iris](https://dev.epicgames.com/documentation/en-us/unreal-engine/migrate-to-iris-in-unreal-engine?application_version=5.7)
- [Epic - Replicating UObjects](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicating-uobjects-in-unreal-engine?application_version=5.7)
- [Epic - Iris Filtering](https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-filtering-in-unreal-engine?application_version=5.7)
- [Epic - Iris Prioritization](https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-prioritization-in-unreal-engine?application_version=5.7)

---

## 2. 기존 기획 검토

### 유사 사례 리스크

기존의 “Iris Arena” 기획은 공개된 BorMor Iris 시리즈와 너무 많이 겹친다.

- 100명 플레이어가 중앙으로 모이는 스트레스 테스트
- 2km x 2km 맵
- 10,000개 나무/엔티티
- UObject inventory replication
- Unreal Insights로 Frame Time, NetBroadcastTickTime 비교

이 시리즈 자체는 좋은 참고 자료지만, 이직용 제출물에서 같은 구성을 그대로 따라가면 “벤치마크를 재현했다”는 인상이 강해진다. 따라서 숫자 규모를 과시하는 데모보다 **게임플레이 문제를 Iris 개념으로 해결하는 데모**로 바꾸는 것이 좋다.

참고:
- [BorMor - Iris 100 Players in One Place](https://bormor.dev/posts/iris-one-hundred-players/)
- [BorMor - The Forest of Ten Thousand Trees](https://bormor.dev/posts/iris-forest-of-ten-thousand-trees/)
- [BorMor - UObject Replication](https://bormor.dev/posts/iris-uobject-replication/)

---

## 3. 신규 데모 기획: "Iris Relay Lab"

### 핵심 목표

> 대규모 수치만 보여주는 벤치마크가 아니라, 실제 멀티플레이 게임에서 생기는 “누가 무엇을 언제 받아야 하는가” 문제를 Iris Filtering, Prioritization, Push Model, Subobject Replication, Seamless Travel로 해결한다.

포트폴리오 관점의 상위 목표는 [Portfolio_Performance_Debugging_Plan.md](./Portfolio_Performance_Debugging_Plan.md)에서 별도로 관리한다. 이 프로젝트의 목표는 “Iris 기능을 써봤다”가 아니라, 같은 조건의 Generic/Iris 실행을 구성하고 로그/프로파일링/NetTrace로 결과를 검증할 수 있음을 보여주는 것이다.

### 데모 콘셉트

플레이어는 협동 작전의 지휘/현장 요원이다. 맵에는 드론, 센서, 보급 상자, 목표 장치, 경보 이벤트가 있고, 각 클라이언트는 자기 역할과 위치에 따라 다른 네트워크 데이터를 받아야 한다.

이 콘셉트는 단순 “많이 복제하기”가 아니라 다음 질문을 보여준다.

- 같은 월드에서 지휘관, 현장 요원, 관전자에게 서로 다른 정보가 보이는가?
- 보급 상자의 내부 아이템 같은 `UObject` 상태가 액터 래퍼 없이 안전하게 복제되는가?
- 관심 없는 센서/드론 상태를 필터링해서 bandwidth와 replication work가 줄어드는가?
- 중요한 경보와 목표 장치는 낮은 빈도 상태보다 우선 복제되는가?
- Lobby -> Operation -> Debrief 이동 중 PlayerState, loadout, inventory가 유지되는가?

추가로 다음 엔지니어링 질문을 계속 유지한다.

- 같은 seed/count/duration 조건에서 Generic/Iris 비교가 성립하는가?
- PIE, command line, ini, dedicated server 실행 경로가 같은 결과를 재현하는가?
- actor count 외에 bandwidth, replicated object count, relevant object count를 남길 수 있는가?
- 결과가 기대와 다를 때 어떤 로그와 프로파일링 자료로 원인을 좁힐 수 있는가?

### 맵 구성

- **Lobby**: 역할 선택, 장비 선택, 작전 브리핑
- **Operation**: 중형 작전 구역. 시야 차단 구조물, 센서 구역, 보급 지점, 목표 장치 포함
- **Debrief**: Seamless Travel 후 결과 요약, replication 통계 비교

---

## 4. 데모 시나리오

### 시나리오 A: Role-Based Interest Filtering

**목적**: Iris Filtering을 포트폴리오 관점에서 직관적으로 보여준다.

상세 구현 계획과 진행 체크리스트는 [ScenarioA_RoleBasedFiltering.md](./ScenarioA_RoleBasedFiltering.md)에서 관리한다.

**흐름**:
1. 플레이어는 Commander, Field Agent, Spectator 중 하나의 역할로 접속한다.
2. Commander는 전체 센서 이벤트와 squad summary를 받는다.
3. Field Agent는 자기 주변 드론/센서/보급 상자만 받는다.
4. Spectator는 지연된 요약 정보만 받는다.
5. UI에서 “Generic / Iris” 실행 결과의 관련 actor 수, replicated object 수, outgoing bandwidth를 비교한다.

**보여줄 Iris 개념**:
- Connection Filter
- Group Filter
- Owner 기반 복제
- Replication Graph 없이 관심 영역을 구성하는 방식

### 시나리오 B: Prioritized Alarm Burst

**목적**: Iris Prioritization과 Push Model의 가치를 보여준다.

**흐름**:
1. 평상시에는 센서, 드론, 보급 상자가 낮은 빈도로 상태를 복제한다.
2. 침입/경보 이벤트가 발생하면 특정 objective와 관련 경보 actor의 우선순위가 상승한다.
3. 경보가 끝나면 다시 낮은 빈도로 돌아간다.
4. UI에서 이벤트 전후의 dirty object 수, replicated object 수, packet size 변화를 표시한다.

**보여줄 Iris 개념**:
- Push-based dirty marking
- Polling fallback과 NetUpdateFrequency 차이
- NetObject prioritizer 설계 포인트
- Gameplay event가 replication 정책을 바꾸는 구조

### 시나리오 C: UObject Loadout and Cargo

**목적**: Actor가 아닌 순수 `UObject` subobject replication을 실전형으로 보여준다.

상세 구현 계획과 진행 체크리스트는 [ScenarioC_UObjectSubobjectReplication.md](./ScenarioC_UObjectSubobjectReplication.md)에서 관리한다.

**흐름**:
1. 플레이어 loadout, 드론 모듈, 보급 상자 cargo slot을 `UObject` 기반 item instance로 구현한다.
2. 아이템은 durability, charge, stack count, owner tag 등 서로 다른 속성을 가진다.
3. 서버에서 아이템 생성/이동/소비가 일어나면 필요한 클라이언트에만 상태가 복제된다.
4. 기존 방식에서는 Actor wrapper 또는 FastArray 중심 설계와 비교하고, Iris에서는 Registered Subobjects List 기반으로 구현한다.

**보여줄 Iris 개념**:
- `AddReplicatedSubObject`
- `bReplicateUsingRegisteredSubObjectList`
- `RegisterReplicationFragments`
- replicated object count budget
- push model property update

### 시나리오 D: Seamless Operation Flow

**목적**: UE 5.7 Iris의 Seamless Travel 지원을 눈에 보이는 기능으로 만든다.

**흐름**:
1. Lobby에서 역할과 loadout 선택
2. Operation 레벨로 Seamless Travel
3. 작전 종료 후 Debrief 레벨로 Seamless Travel
4. PlayerState, score, loadout, cargo history가 유지되는지 검증
5. travel 전후 NetRefHandle 재생성/상태 유지 관련 로그를 개발자 UI에 표시한다.

**보여줄 Iris 개념**:
- Seamless Travel 중 replicated actor/state 처리
- PlayerState 유지
- subobject reference 재매핑 주의점
- travel 전후 lifecycle 로그

### 시나리오 E: Network Fault and Trace Console

**목적**: 데모가 단순 구현이 아니라 측정 가능한 네트워크 엔지니어링 결과물임을 보여준다.

**흐름**:
1. Normal, Packet Loss, Latency, Buffer Bloat 프로파일을 선택한다.
2. 같은 작전 상황을 Generic / Iris 설정으로 실행한다.
3. Unreal Insights, CSV Profiling, NetTrace 결과를 저장한다.
4. Debrief 화면에서 run metadata와 핵심 수치를 요약한다.

**보여줄 역량**:
- 재현 가능한 테스트 하네스
- 네트워크 품질 변화에 대한 복제 정책 검증
- 수치 기반 의사결정

---

## 5. 프로젝트 구조 제안

```text
IrisDemo/
├── Docs/
│   ├── start.md
│   ├── planning.md
│   └── Iris기본지식.md
├── Content/
│   ├── Maps/
│   │   ├── Lobby.umap
│   │   ├── Operation.umap
│   │   └── Debrief.umap
│   ├── Blueprints/
│   │   ├── Characters/
│   │   ├── GameModes/
│   │   ├── Interactables/
│   │   └── UI/
│   └── ...
├── Source/
│   └── IrisDemo/
│       ├── IrisDemo.Build.cs
│       ├── GameModes/
│       ├── Characters/
│       ├── Inventory/
│       │   ├── IrisItemInstance.h/cpp
│       │   └── IrisInventoryComponent.h/cpp
│       ├── Operation/
│       │   ├── RelaySensorActor.h/cpp
│       │   ├── RelayDroneActor.h/cpp
│       │   └── RelayObjectiveActor.h/cpp
│       ├── Replication/
│       │   ├── RelayReplicationPolicy.h/cpp
│       │   ├── RelayNetFilterConfig.h/cpp
│       │   └── RelayNetStatsSubsystem.h/cpp
│       └── UI/
├── Config/
│   └── DefaultEngine.ini
└── IrisDemo.uproject
```

---

## 6. 설정 가이드

### `.uproject`

```json
{
    "Plugins": [
        {
            "Name": "Iris",
            "Enabled": true
        }
    ]
}
```

### `IrisDemo.Build.cs`

```csharp
public IrisDemo(ReadOnlyTargetRules Target) : base(Target)
{
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    SetupIrisSupport(Target);
}
```

### `.Target.cs`

UE 5.7.1 기준으로 `TargetRules.bUseIris`는 존재하지 않는다. Target.cs에는 일반 Target 설정만 두고, Iris 활성화는 `.uproject`, `Build.cs`, `DefaultEngine.ini`, 실행 인자로 관리한다.

### `DefaultEngine.ini`

```ini
[SystemSettings]
net.SubObjects.DefaultUseSubObjectReplicationList=1
net.Iris.UseIrisReplication=1

; PushModelMode=1을 쓸 때 함께 활성화
net.IsPushModelEnabled=1
net.Iris.PushModelMode=1

; 개발 중 Iris 로그 확인용. 필요할 때만 켠다.
; LogIris=VeryVerbose
; LogIrisReplication=VeryVerbose
```

### 실행 인자

```text
-UseIrisReplication=1
-UseIrisReplication=0
```

IrisDemo는 `DefaultEngine.ini`와 로컬 실행 배치 기준으로 Iris를 기본값으로 둔다. Generic 실행은 `--no-iris` 또는 `-UseIrisReplication=0`을 명시해 기존 replication path를 비교하기 위한 control run으로 사용한다. 비교 실행은 같은 맵, 같은 bot seed, 같은 network emulation profile, 같은 run duration을 사용한다.

---

## 7. 측정 지표

세부 측정/디버깅 보강 계획은 [Portfolio_Performance_Debugging_Plan.md](./Portfolio_Performance_Debugging_Plan.md)를 따른다.

`PerformanceRuns.md`는 프로그램이 자동 출력하는 원본 산출물이 아니라, 실행 후 `Saved/ScenarioA/Runs/<RunId>/`에 남는 `run.json`, `client_snapshots.csv`, `server_network_metrics.csv`를 사람이 같은 RunId 기준으로 대조해 정리하는 성능 비교 문서다. 자동 생성 대상은 RunId 폴더의 JSON/CSV이고, `PerformanceRuns.md`는 그 결과를 포트폴리오 설명용 표와 해석으로 옮기는 문서로 다룬다.

- Server Frame Time
- NetBroadcastTickTime
- Outgoing Bandwidth
- Replicated Object Count
- Relevant Object Count per Connection
- Dirty Object Count
- Packet Size
- RPC Count
- Seamless Travel duration
- Travel 전후 PlayerState/loadout/cargo 유지 성공 여부

---

## 8. 학습 순서

자세한 사전 지식은 [Iris기본지식.md](./Iris기본지식.md)에 분리했다.

1. 기존 UE replication 기초: Actor replication, RPC, `GetLifetimeReplicatedProps`, Role/Authority
2. Iris 활성화와 실행 스위치: plugin, Build.cs, Target.cs, ini, command line
3. Iris 핵심 모델: Replication System, Replication State, Fragment, NetRefHandle
4. Filtering/Prioritization: Replication Graph 대체 관점으로 이해
5. Push Model과 dirty marking
6. Registered Subobjects List와 UObject replication
7. Seamless Travel에서 유지할 상태와 재생성할 상태 구분
8. Unreal Insights, CSV Profiling, NetTrace 측정 자동화

---

## 9. 다음 단계

1. Scenario A에서 actor count 외의 outgoing bandwidth 또는 replicated object count 중 최소 1개를 RunId 폴더에 기록
2. Unreal Insights / CSV Profiling / NetTrace 수집 경로를 정리하고, RunId 폴더의 JSON/CSV 산출물을 확인한 뒤 `PerformanceRuns.md`에 비교 결과와 해석을 수동 기록
3. `DebuggingNotes.md`에 설정/실행 모드 차이와 실패 로그 분석을 기록
4. Scenario A의 A7 마무리 문서화 완료
5. Scenario C의 UObject loadout/cargo를 최소 기능으로 연결하고 `OnRep` 로그를 listen/dedicated server에서 확인
6. Scenario B의 prioritization/push model을 성능 측정과 연결
7. Scenario D/E를 통해 seamless travel과 네트워크 fault profile 검증으로 확장
