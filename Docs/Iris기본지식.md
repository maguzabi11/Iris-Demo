# Iris 기본 지식 정리

이 문서는 Iris 데모를 만들기 전에 알아야 할 개념을 정리한 학습 노트다. 목표는 “Iris가 빠르다”를 외우는 것이 아니라, 어떤 복제 문제를 어떤 Iris 개념으로 풀어야 하는지 판단할 수 있게 만드는 것이다.

## 1. 먼저 알아야 할 Unreal 네트워크 기본

### Server Authority

Unreal 멀티플레이의 기본은 서버 권위 모델이다. 서버가 진짜 게임 상태를 가지고, 클라이언트는 입력을 보내거나 서버가 보낸 상태를 표시한다.

데모 구현 전 확인할 것:
- replicated actor는 서버에서 spawn한다.
- gameplay state 변경은 서버에서 확정한다.
- 클라이언트 예측이 필요한 이동/입력과 단순 상태 복제는 구분한다.

### Actor Replication

기존 Unreal replication의 기본 단위는 `AActor`다.

필수 개념:
- `bReplicates = true`
- `UPROPERTY(Replicated)`
- `GetLifetimeReplicatedProps`
- `DOREPLIFETIME`
- `OnRep_*`
- Server/Client/NetMulticast RPC
- `NetUpdateFrequency`
- relevancy와 dormancy

Iris를 써도 replicated property와 RPC의 표면 문법은 많이 유지된다. 다만 내부 처리와 확장 지점이 다르다.

## 2. Iris의 위치

Iris는 Unreal의 차세대 replication system이다. 기존 Generic Replication과 함께 존재하는 opt-in 시스템이며, 프로젝트가 명시적으로 활성화해야 한다.

Epic 문서 기준 핵심 표현:
- 더 큰 interactive world
- 더 높은 player count
- 더 낮은 server cost
- 기존 replicated property/RPC와 최대한 호환

중요한 점:
- Iris를 켠다고 모든 코드가 자동으로 최적화되는 것은 아니다.
- 기존 Replication Graph와 Iris는 같은 NetDriver에서 같이 쓰는 방식이 아니다.
- Replication Graph의 역할은 Iris Filtering과 Prioritization으로 옮겨 생각해야 한다.

## 3. Iris 활성화 체크리스트

### `.uproject`

```json
{
    "Name": "Iris",
    "Enabled": true
}
```

### `*.Build.cs`

```csharp
SetupIrisSupport(Target);
```

### `*.Target.cs`

```csharp
bUseIris = true;
```

### `DefaultEngine.ini`

```ini
[SystemSettings]
net.SubObjects.DefaultUseSubObjectReplicationList=1
net.Iris.UseIrisReplication=1
net.IsPushModelEnabled=1
net.Iris.PushModelMode=1
```

### 실행 스위치

```text
-UseIrisReplication=1
-UseIrisReplication=0
```

포트폴리오 데모에서는 이 스위치를 이용해 같은 시나리오를 Generic/Iris로 반복 실행할 수 있어야 한다.

## 4. Iris 핵심 용어

### Replication System

Iris의 중심 시스템이다. replicated object의 상태를 관리하고, 어떤 connection에 어떤 object를 보낼지 filtering/prioritization/dirty tracking을 거쳐 결정한다.

### NetRefHandle

Iris가 네트워크 복제 대상 object를 식별하는 handle이다. Actor, Component, Subobject가 Iris 내부에서 복제 대상으로 등록되면 handle을 가진다고 보면 된다.

### Replication State

복제할 데이터의 상태 표현이다. Iris는 gameplay object에서 값을 직접 매번 긁어오는 방식보다, 복제 상태를 별도로 관리하고 quantized state를 기반으로 동작한다.

### Replication Fragment

object의 어떤 property/RPC 묶음을 Iris replication state로 등록하는 단위다. `AActor`는 많은 부분이 자동 처리되지만, 순수 `UObject`는 `RegisterReplicationFragments`를 이해해야 한다.

### Net Serializer

타입을 네트워크로 직렬화하는 규칙이다. 일반적인 `UPROPERTY` 타입은 기본 지원되지만, custom `NetSerialize` struct는 Iris에서 경고가 나거나 별도 설정이 필요할 수 있다.

## 5. Filtering

Filtering은 “이 object가 이 connection에 복제되어야 하는가?”를 결정한다.

데모에서 보여줄 예:
- Commander는 전체 작전 요약을 받는다.
- Field Agent는 주변 센서와 자기 squad 정보만 받는다.
- Spectator는 지연된 summary만 받는다.

Iris에서 봐야 할 필터 유형:
- Owner filter
- Connection filter
- Group filter
- NetObject filter
- distance/spatial filter

Replication Graph를 쓰던 사고방식에서는 “노드에 actor를 넣는다”가 핵심이었다. Iris에서는 “object가 어떤 connection 집합에 허용되는가”와 “필터 정책을 어떻게 구성하는가”로 사고를 바꿔야 한다.

## 6. Prioritization

Prioritization은 “보낼 수 있는 것 중 무엇을 먼저 보낼 것인가?”를 결정한다.

데모에서 보여줄 예:
- 평상시 드론 배터리, 센서 온도, cargo durability는 낮은 우선순위
- 경보 발생 시 objective state, alarm event, affected sensor는 높은 우선순위
- bandwidth 제한 상황에서 중요한 state가 먼저 도착하는지 확인

측정할 것:
- prioritized object count
- packet size
- 경보 발생 후 first visible update latency
- 낮은 우선순위 object의 update delay

## 7. Push Model과 Polling

Iris는 push 기반을 지향한다. 즉, 값이 바뀐 시점을 명확히 표시해서 replication system이 불필요한 property polling을 줄이도록 만드는 것이 핵심이다.

주의할 점:
- push model을 켰다고 자동으로 모든 property가 올바르게 dirty 처리되는 것은 아니다.
- gameplay code에서 값 변경 후 dirty marking을 빠뜨리면 클라이언트가 업데이트를 못 받을 수 있다.
- push model을 쓰지 않는 object는 `NetUpdateFrequency` 기반 polling fallback을 이해해야 한다.

학습 목표:
- 어떤 property가 push-based인지 구분한다.
- 값 변경 경로를 서버 함수로 모은다.
- 변경 함수 안에서 dirty marking을 일관되게 처리한다.

## 8. UObject/Subobject Replication

Iris 데모에서 가장 중요한 차별화 포인트다.

### 기본 원칙

순수 `UObject`는 단독으로 네트워크에 떠다니는 replicated actor가 아니다. Actor 또는 ActorComponent의 subobject로 복제된다.

필수 체크리스트:
- owning actor 또는 component가 replicate된다.
- `UObject`가 `IsSupportedForNetworking()`을 override한다.
- `UObject`에 `GetLifetimeReplicatedProps`가 있다.
- 서버에서 생성한다.
- outer가 owning actor/component로 잡힌다.
- `AddReplicatedSubObject`로 등록한다.
- Iris에서는 Registered Subobjects List를 사용한다.
- 필요 시 `RegisterReplicationFragments`를 구현한다.

### Registered Subobjects List

Iris는 virtual `ReplicateSubobjects` 방식이 아니라 Registered Subobjects List를 요구한다.

Actor/Component constructor에서:

```cpp
bReplicateUsingRegisteredSubObjectList = true;
```

서버에서 subobject 생성 후:

```cpp
AddReplicatedSubObject(ItemInstance);
```

제거 시:

```cpp
RemoveReplicatedSubObject(ItemInstance);
```

### 데모 적용 예

`UIrisItemInstance`:
- item id
- durability
- charge
- stack count
- item state enum

`UIrisInventoryComponent`:
- item instance array
- add/remove/move item server API
- item별 replication condition
- cargo box, drone, player loadout이 같은 item instance 기반 구조 공유

## 9. Seamless Travel

UE 5.7 릴리즈 노트에서 Iris seamless travel support가 명시되었다. 이 기능은 포트폴리오 데모에서 좋은 차별점이 된다.

데모에서 검증할 것:
- Lobby에서 선택한 role/loadout이 Operation으로 유지되는가?
- Operation 결과와 cargo history가 Debrief로 유지되는가?
- travel 전후 PlayerState가 유지되는가?
- subobject reference가 travel 후 올바르게 재연결되는가?
- travel 중 임시 actor와 persistent state를 구분했는가?

구현 원칙:
- 유지할 정보는 PlayerState, GameInstance, 또는 seamless travel actor 목록으로 명확히 분리한다.
- Operation 맵 전용 actor는 travel 후 재생성되는 것으로 설계한다.
- loadout/cargo 같은 subobject는 소유 주체와 lifetime을 명확히 한다.

## 10. 측정과 디버깅

포트폴리오 데모는 “잘 된다”보다 “측정했다”가 중요하다.

필수 도구:
- Unreal Insights
- CSV Profiling
- NetTrace
- server log
- run metadata json/csv

기록할 값:
- engine version
- map/scenario name
- Generic/Iris 여부
- bot/player count
- seed
- network emulation profile
- server tick rate
- average/p95 server frame time
- average/p95 NetBroadcastTickTime
- outgoing bandwidth
- replicated object count
- relevant object count per connection
- dirty object count

실행 조건:
- 같은 seed
- 같은 duration
- 같은 bot script
- 같은 server tick rate
- 같은 network emulation profile

## 11. 흔한 함정

- `bUseIris` 또는 `SetupIrisSupport(Target)`를 빠뜨리고 ini만 바꾼다.
- Iris와 Replication Graph를 동시에 쓰는 구조로 설계한다.
- `UObject`를 생성했지만 `AddReplicatedSubObject`를 호출하지 않는다.
- subobject outer가 owning actor/component가 아니다.
- push model dirty marking을 빠뜨린다.
- actor wrapper를 남발해서 Iris subobject replication의 장점을 보여주지 못한다.
- benchmark 숫자만 있고 왜 빨라졌는지 설명하지 못한다.
- profiling 조건이 매번 달라 A/B 비교가 성립하지 않는다.

## 12. 추천 학습 순서

1. 기존 Actor replication과 RPC를 작은 actor 하나로 복습한다.
2. Iris를 켜고 같은 actor가 동작하는지 확인한다.
3. Generic/Iris 실행 스위치를 만든다.
4. Actor 10개, client 2개로 filtering을 실험한다.
5. 경보 actor 하나로 prioritization과 push model을 실험한다.
6. `UObject` item instance 하나를 player component의 subobject로 복제한다.
7. item array, add/remove/move, owner-only 조건을 붙인다.
8. Lobby -> Operation -> Debrief seamless travel을 연결한다.
9. Unreal Insights/CSV/NetTrace 자동 수집을 붙인다.

## 13. 참고 링크

- [Epic - Unreal Engine 5.7 Release Notes](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-7-release-notes?application_version=5.7)
- [Epic - Iris Replication System](https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-replication-system-in-unreal-engine?application_version=5.7)
- [Epic - Migrate to Iris](https://dev.epicgames.com/documentation/en-us/unreal-engine/migrate-to-iris-in-unreal-engine?application_version=5.7)
- [Epic - Replicating UObjects](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicating-uobjects-in-unreal-engine?application_version=5.7)
- [Epic - Iris Filtering](https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-filtering-in-unreal-engine?application_version=5.7)
- [Epic - Iris Prioritization](https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-prioritization-in-unreal-engine?application_version=5.7)
- [Epic - Console Commands for Network Debugging](https://dev.epicgames.com/documentation/en-us/unreal-engine/console-commands-for-network-debugging-in-unreal-engine?application_version=5.7)
