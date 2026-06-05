# Scenario C 상세 기획: UObject/Subobject Replication

이 문서는 `Iris Relay Lab`의 세 번째 구현 단위인 Scenario C를 작은 빌드 가능 단계로 나누기 위한 작업 문서다.

## 1. 현재 상태

- 기준일: 2026-06-05
- 엔진 기준: UE 5.7.1
- 목표 시나리오: actor/component가 소유한 순수 `UObject` cargo item을 registered subobject list로 복제
- 현재 단계: C0/C1/C2/C3 최소 검증 완료, C4 inventory component 최소 구현 빌드 검증 완료

이미 확인한 기반 상태:

- [x] `Config/DefaultEngine.ini`에 `net.SubObjects.DefaultUseSubObjectReplicationList=1`
- [x] `Config/DefaultEngine.ini`에 `net.Iris.UseIrisReplication=1`
- [x] `Source/IrisDemo/IrisDemo.Build.cs`에서 `SetupIrisSupport(Target)` 유지
- [x] UE 5.7.1 헤더 기준 `AddReplicatedSubObject`, `RemoveReplicatedSubObject`, `RegisterReplicationFragments`, `FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject` 사용 가능 확인
- [x] Scenario C 최소 C++ class 골격 작성
- [x] UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공
- [x] `URelayCargoInventoryComponent` 기반 item array 소유 구조 빌드 성공

## 2. 시나리오 목표

Scenario C는 Actor wrapper만 늘리는 방식이 아니라, actor/component가 소유한 순수 `UObject` 상태를 subobject로 복제하는 구조를 보여준다.

첫 완료 기준은 다음 로그를 클라이언트에서 확인하는 것이다.

```text
Relay cargo item replicated: ItemId=...
Relay cargo station cargo item pointer replicated: ...
```

## 3. 최소 구현 범위

### 포함

- `URelayCargoItem`: 순수 `UObject` item instance
- `ARelayCargoStationActor`: cargo item을 outer/subobject로 소유하는 replicated actor
- registered subobject list 사용:
  - actor constructor에서 `bReplicateUsingRegisteredSubObjectList = true`
  - 서버에서 `NewObject<URelayCargoItem>(this, ...)`
  - 서버에서 `AddReplicatedSubObject(CargoItem)`
  - EndPlay에서 `RemoveReplicatedSubObject(CargoItem)`
- `URelayCargoItem::IsSupportedForNetworking()`
- `URelayCargoItem::GetLifetimeReplicatedProps`
- `URelayCargoItem::RegisterReplicationFragments`
- 서버 주기 update와 client `OnRep` 로그
- Scenario A 측정 오염 방지를 위해 기본 비활성화

### 제외

- item array와 inventory component
- owner-only 조건
- cargo 이동/소비 RPC
- FastArray 비교
- role-based filtering과 subobject filtering 결합
- push model dirty marking 세부 검증
- seamless travel 유지 검증

## 4. 구현 class

| Class | 역할 |
|-------|------|
| `URelayCargoItem` | `ItemId`, `ItemTag`, `StackCount`, `Durability`, `Charge`, `ItemState`, `LastUpdateSequence`를 복제하는 순수 `UObject` |
| `ARelayCargoStationActor` | station 식별, 위치, 표시를 담당하고 cargo item 생성/갱신은 inventory component에 위임하는 테스트 actor |
| `URelayCargoInventoryComponent` | cargo item collection 소유자. station actor에 부착되어 item array 생성/갱신/등록 lifecycle을 담당 |
| `AIrisDemoGameMode` | `-ScenarioCEnableCargoSubobjects=1`일 때 cargo station을 spawn하고 기존 update timer에서 cargo item 상태를 갱신 |

## 5. 실행 방법

Scenario C는 기본적으로 꺼져 있다. Scenario A heavy/baseline 수치를 유지하기 위해 별도 실행 인자로 켠다.

```text
-ScenarioCEnableCargoSubobjects=1
```

선택 옵션:

```text
-ScenarioCCargoStationCount=1
-ScenarioCCargoItemCount=1
```

PIE나 local binary에서 켠 뒤 Output Log에서 다음 항목을 확인한다.

- `Scenario C cargo stations spawned`
- `Scenario C cargo item registered`
- `Relay cargo station replicated`
- `Relay cargo item replicated`

## 6. 단계별 체크리스트

### C0. 기반 확인

- [x] registered subobject list 설정 확인
- [x] UE 5.7.1 헤더에서 subobject API 확인
- [x] Scenario A와 독립적으로 켤 수 있는 option 결정

검증 메모:

- 2026-06-03: UE 5.7.1 헤더에서 `AActor::AddReplicatedSubObject`, `AActor::RemoveReplicatedSubObject`, `UObject::RegisterReplicationFragments`, `FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject` 확인.

### C1. 최소 replicated UObject

- [x] `URelayCargoItem` 추가
- [x] `IsSupportedForNetworking()` override
- [x] `GetLifetimeReplicatedProps` 작성
- [x] `RegisterReplicationFragments`에서 `FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject` 호출
- [x] `OnRep_LastUpdateSequence` 로그 작성

검증 메모:

- 2026-06-03: `Tools\BuildEditor.bat`로 UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.

### C2. Owning actor와 registered subobject list

- [x] `ARelayCargoStationActor` 추가
- [x] `bReplicateUsingRegisteredSubObjectList = true`
- [x] 서버에서 `URelayCargoItem` 생성
- [x] 서버에서 `AddReplicatedSubObject(CargoItem)` 호출
- [x] `CargoItem` pointer property 복제
- [x] EndPlay에서 `RemoveReplicatedSubObject(CargoItem)` 호출

검증 메모:

- 2026-06-03: `Tools\BuildEditor.bat`로 UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공. PIE 또는 local binary client `OnRep` 확인은 다음 단계.

### C3. 런타임 검증

- [x] 에디터에서 Scenario C 기본 구현 확인
- [x] listen server + 1 client PIE에서 `Relay cargo item replicated` 확인
- [x] dedicated server + 1 client local binary에서 `Relay cargo item replicated` 확인
- [x] Generic/Iris 양쪽에서 같은 item update sequence 수신 여부 확인
- [x] client snapshot 또는 CSV에 cargo item count/sequence 추가

검증 메모:

- 2026-06-04: 에디터에서 Scenario C 기본 구현 확인 완료. 이후 수동 Output Log 확인만으로 끝나지 않도록 기존 `IrisRelayLogBaselineSnapshot` 경로에 `CargoStationCount`, `CargoItemCount`, `CargoStationLastSequence`, `CargoItemLastSequence`를 추가한다.
- 2026-06-04: listen server + 1 client PIE, dedicated server + 1 client local binary, Generic/Iris 양쪽에서 cargo item replication과 snapshot cargo count/sequence 확인 완료.

### C4. Inventory component와 item array 확장

- [x] `URelayCargoInventoryComponent` 추가
- [x] `ARelayCargoStationActor::CargoItem` 단일 포인터를 component 소유 item collection으로 이전
- [x] station actor는 inventory component를 기본 subobject로 소유하고, cargo item 생성/갱신 요청만 위임
- [x] inventory component는 서버에서 `URelayCargoItem` array를 생성/보관
- [x] registered subobject list 등록/해제는 actor API를 통해 수행하되, item lifecycle 판단은 inventory component가 담당
- [x] snapshot/CSV에 component 기반 item count와 max/last item sequence를 유지
- [x] `-ScenarioCCargoItemCount` 같은 item 수 option 추가 여부 결정
- [x] UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 확인

계획 메모:

- C4는 actor가 직접 `TArray<URelayCargoItem*>`를 갖는 구조보다 `URelayCargoInventoryComponent`를 두는 구조를 우선한다.
- 이유는 `ARelayCargoStationActor`를 위치/식별/시나리오 spawn을 담당하는 wrapper actor로 유지하고, cargo collection의 생성, 갱신, snapshot 집계, subobject 등록 lifecycle을 별도 replication owner 단위로 분리하기 위해서다.
- 이후 owner-only, role-based filtering, item 이동/소비 API, 다른 station actor 재사용으로 확장할 때 actor class를 계속 비대하게 만들지 않고 component 단위로 정책을 붙일 수 있다.
- UE subobject 등록 API는 actor 경로를 사용하므로, component가 직접 복제 등록 API의 최종 소유자가 되는 것이 아니라 owner actor를 통해 `AddReplicatedSubObject`/`RemoveReplicatedSubObject`를 호출하는 helper를 제공하는 방식으로 검증한다.

검증 메모:

- 2026-06-05: `URelayCargoInventoryComponent` 추가. station actor는 component default subobject를 소유하고, component가 owner actor의 `AddReplicatedSubObject`/`RemoveReplicatedSubObject`를 통해 `URelayCargoItem` array를 등록/해제한다.
- 2026-06-05: `-ScenarioCCargoItemCount` 옵션 추가. client snapshot 집계는 component의 item array 전체를 순회해 `CargoItemCount`, `CargoItemLastSequence`를 계산한다.
- 2026-06-05: `Tools\BuildEditor.bat`로 UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공.

## 7. 결정 로그

| 날짜 | 결정 | 근거 | 후속 작업 |
|------|------|------|-----------|
| 2026-06-03 | Scenario C 최소 골격은 Scenario A와 별도 option으로 켠다. | Scenario A bandwidth 측정은 이미 마감했으므로 새 replicated actor/subobject가 기존 수치를 오염시키면 안 됨 | `-ScenarioCEnableCargoSubobjects=1` 실행으로 별도 검증 |
| 2026-06-03 | 첫 구현은 actor 1개와 cargo item 1개로 제한한다. | 목표는 inventory 전체가 아니라 registered subobject replication의 빌드 가능한 최소 단위 증명 | 다음 단계에서 item array, owner-only, 이동/소비 API 확장 |
| 2026-06-04 | GameMode의 URL/command line option source helper는 Scenario A 전용 이름 대신 Scenario 공통 이름을 사용한다. | Scenario C option도 같은 파싱 경로를 쓰므로 `GetScenarioAOptionSourceLabel(ScenarioCEnableSource)` 같은 호출은 의도를 흐림 | Scenario별 설정이 더 늘어나면 GameMode option 적용 함수를 Scenario A/C 단위로 분리 |
| 2026-06-04 | Scenario C 검증은 `OnRep` 로그와 snapshot CSV를 같이 본다. | subobject pointer replication과 subobject property replication은 각각 확인해야 하므로 수동 로그만으로는 반복 검증이 약함 | PIE/local binary에서 snapshot row의 cargo count/sequence를 확인 |
| 2026-06-04 | C4 item array 확장은 `ARelayCargoStationActor` 직접 배열이 아니라 `URelayCargoInventoryComponent` 소유 구조로 진행한다. | station actor는 scenario spawn 지점과 replicated wrapper 역할에 집중하고, cargo collection lifecycle과 향후 policy 결합 지점을 component로 분리하는 편이 확장성이 좋음 | component가 owner actor를 통해 registered subobject list에 item들을 등록/해제하는 최소 구조 검증 |
| 2026-06-05 | `-ScenarioCCargoItemCount` 옵션을 추가한다. | component array 구조가 item 1개만으로는 C2와 차이가 작아 snapshot에서 다중 item 집계를 확인하기 어려움 | PIE/local binary에서 item count 2 이상으로 replication 로그와 snapshot row 확인 |

## 8. 다음 작업 후보

1. PIE 또는 local binary에서 `-ScenarioCCargoItemCount=2` 이상으로 component item array replication 확인
2. Scenario C 전용 console command가 필요한지 판단
3. owner-only 또는 role-based policy와 cargo item visibility 결합
4. cargo 이동/소비 RPC 또는 FastArray 비교 시나리오 확장
