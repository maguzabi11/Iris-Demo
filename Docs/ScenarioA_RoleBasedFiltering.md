# Scenario A 상세 기획: Role-Based Interest Filtering

이 문서는 `Iris Relay Lab`의 첫 번째 구현 단위인 Scenario A를 작은 빌드 가능 단계로 나누고, 진행 상황을 계속 체크하기 위한 작업 문서다.

## 1. 현재 상태

- 기준일: 2026-05-15
- 엔진 기준: UE 5.7.1
- 목표 시나리오: 역할별 관심 정보 차등 복제
- 현재 단계: Iris 프로젝트 설정은 완료했고, Scenario A gameplay/replication 골격을 설계하고 구현할 차례

이미 확인한 기반 상태:

- [x] `IrisDemo.uproject`에 `Iris` plugin 활성화
- [x] `Source/IrisDemo/IrisDemo.Build.cs`에서 `SetupIrisSupport(Target)` 유지
- [x] `Source/IrisDemo.Target.cs`, `Source/IrisDemoEditor.Target.cs`에 존재하지 않는 `bUseIris` 설정 없음
- [x] `Config/DefaultEngine.ini`에 `net.Iris.UseIrisReplication=1` 설정
- [x] `Config/DefaultEngine.ini`에 registered subobject list/push model 관련 기본 설정 추가
- [ ] Scenario A 전용 C++ class 골격 작성
- [ ] 역할 선택/할당 flow 작성
- [ ] 역할별 replicated actor visibility 차이 구현
- [ ] Generic/Iris 비교 실행 절차 작성
- [ ] 측정 UI 또는 로그 요약 구현
- [ ] UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 확인

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

예상 class:

- `ERelayOperatorRole`
- `ARelayPlayerState`
- `ARelayGameMode`
- `ARelayPlayerController`

역할 할당 방식은 초기에는 간단하게 시작한다.

1. 서버가 접속 순서에 따라 역할을 부여한다.
2. 개발용 console command 또는 UI로 역할을 변경한다.
3. 역할 변경 시 서버가 filtering 대상 갱신을 요청한다.

나중에 Lobby가 생기면 역할 선택 UI로 교체한다.

### 4.2 테스트 actor 모델

Scenario A actor는 gameplay보다 replication 정책 검증에 집중한다.

공통 속성:

- `ZoneId`
- `DebugName`
- `LastUpdateSequence`
- `bScenarioAEnabled`

actor별 replicated 속성:

- Sensor: `AlertLevel`, `bTriggered`
- Drone: `BatteryPercent`, `EDroneRelayState`
- SupplyCrate: `StockCount`, `bReserved`
- Summary: `KnownAlertCount`, `KnownDroneCount`, `KnownSupplyCount`

서버는 일정 주기 또는 입력으로 sequence 값을 올리고, 클라이언트는 자신에게 도착한 마지막 sequence를 표시한다.

### 4.3 Filtering 정책

정책은 처음부터 엔진 내부를 과하게 추상화하지 않고, 다음 두 단계로 나눈다.

1. Baseline relevancy
   - 기존 actor replication/relevancy로 역할별 차이를 최소한 재현한다.
   - owner-only, distance relevancy, dormancy 사용 가능성을 확인한다.

2. Iris filtering
   - UE 5.7.1에서 사용 가능한 공식 Iris filtering API를 기준으로 connection/group filter 적용 지점을 찾는다.
   - role/zone을 group 또는 connection 조건으로 매핑한다.
   - Replication Graph는 도입하지 않는다.

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

- [ ] `ERelayOperatorRole` 정의
- [ ] 역할을 저장할 `PlayerState` 또는 기존 PlayerState 확장 방향 결정
- [ ] 서버 권위 역할 할당 함수 작성
- [ ] 클라이언트에서 현재 역할 확인 가능
- [ ] 역할 변경 시 로그 출력
- [ ] 2 client 이상 PIE에서 역할이 다르게 보이는지 확인

완료 기준:

- Commander/FieldAgent/Spectator 세 역할을 서버가 구분하고, 각 클라이언트가 자기 역할을 표시할 수 있다.

### A2. Scenario A 테스트 actor

- [ ] `RelaySensorActor` C++ class 작성
- [ ] `RelayDroneActor` C++ class 작성
- [ ] `RelaySupplyCrateActor` C++ class 작성
- [ ] summary actor 또는 GameState summary 작성
- [ ] 서버에서 테스트 actor를 deterministic하게 spawn
- [ ] actor별 replicated property와 `OnRep` 로그 작성
- [ ] actor count/zone 배치 seed를 설정으로 분리

완료 기준:

- 서버 1개, 클라이언트 2개 이상에서 actor 상태 변경 sequence가 클라이언트에 복제된다.

### A3. Baseline 비교 기준

- [ ] Generic replication 실행 인자 정리: `-UseIrisReplication=0`
- [ ] Iris 실행 인자 정리: `-UseIrisReplication=1`
- [ ] 같은 seed/count/duration으로 실행하는 절차 작성
- [ ] Generic mode에서 역할별 수신 actor count 기록
- [ ] Iris mode에서 역할별 수신 actor count 기록
- [ ] 비교 결과를 `Docs/ScenarioA_RoleBasedFiltering.md`에 기록

완료 기준:

- Generic/Iris 실행 조건을 동일하게 맞춘 비교 표가 최소 1회 기록된다.

### A4. Role-Based Filtering 구현

- [ ] UE 5.7.1 공식 문서/헤더 기준으로 Iris filtering 적용 API 확인
- [ ] Commander connection에 전체 summary/detail 허용
- [ ] FieldAgent connection에 zone/distance 기반 detail 허용
- [ ] Spectator connection에 delayed summary만 허용
- [ ] 역할 변경 시 filter membership 갱신
- [ ] actor spawn/despawn 시 filter membership 갱신
- [ ] filtering 실패 시 fallback 로그 또는 ensure 추가

완료 기준:

- 같은 서버 상태에서 역할별 클라이언트가 수신하는 detailed actor set이 다르고, 의도하지 않은 상세 actor가 Spectator에 복제되지 않는다.

### A5. Debug UI/로그

- [ ] 현재 role 표시
- [ ] local received sensor/drone/crate count 표시
- [ ] zone별 received count 표시
- [ ] last sequence 표시
- [ ] Generic/Iris mode 표시
- [ ] server summary log 출력
- [ ] 결과 캡처용 console command 또는 key binding 작성

완료 기준:

- 에디터 PIE 화면 또는 로그만 보고도 역할별 관심 필터링 결과를 설명할 수 있다.

### A6. 검증

- [ ] UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드 성공
- [ ] listen server + 2 clients PIE 검증
- [ ] dedicated server + 2 clients 실행 검증
- [ ] Generic/Iris 양쪽에서 실행 성공
- [ ] 역할별 expected/actual actor count 표 작성
- [ ] false positive 복제 목록 확인
- [ ] bandwidth 또는 replicated object count 수집 가능성 확인

완료 기준:

- Scenario A를 2분 안에 재현할 수 있고, 결과를 문서의 표로 설명할 수 있다.

### A7. 마무리 문서화

- [ ] 구현 class 목록 추가
- [ ] 실행 방법 추가
- [ ] 측정 결과 표 추가
- [ ] UE 5.7.1 Iris filtering 제약/주의점 추가
- [ ] 다음 시나리오로 넘길 TODO 분리
- [ ] `Docs/Iris기본지식.md`에 새로 확인한 Iris 사실 반영

완료 기준:

- 제3자가 문서만 보고 Scenario A의 목적, 실행법, 결과 해석, 남은 한계를 이해할 수 있다.

## 6. 결과 기록 표

| 날짜 | 모드 | 서버 형태 | 클라이언트 수 | Seed | Commander 수신 | FieldAgent 수신 | Spectator 수신 | 메모 |
|------|------|-----------|---------------|------|-----------------|------------------|-----------------|------|
| - | Generic | - | - | - | - | - | - | 아직 미측정 |
| - | Iris | - | - | - | - | - | - | 아직 미측정 |

## 7. 결정 로그

| 날짜 | 결정 | 근거 | 후속 작업 |
|------|------|------|-----------|
| 2026-05-15 | Scenario A는 별도 문서에서 체크리스트를 관리한다. | `planning.md`는 전체 기획 문서라 구현 진행 체크를 계속 누적하기에 길어질 수 있음 | 구현이 진행될 때 이 문서를 함께 갱신 |
| 2026-05-15 | 첫 구현 범위에서 UObject/subobject, prioritization, seamless travel은 제외한다. | Scenario A의 핵심은 role/connection별 filtering 결과를 먼저 증명하는 것 | 이후 Scenario C/B/D에서 확장 |

## 8. 다음 작업 후보

가장 작은 다음 커밋 후보:

1. `ERelayOperatorRole`과 role-aware `PlayerState` 골격 추가
2. 접속 순서 기반 역할 자동 할당
3. local debug log로 역할 확인
4. UE 5.7.1 editor build 검증

이 커밋이 통과하면 `A1. 역할 골격` 체크리스트를 갱신한다.
