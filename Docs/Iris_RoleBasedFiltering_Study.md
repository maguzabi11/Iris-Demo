# Iris Replication 기능 학습: 역할 기반 필터링 (Role-Based Filtering)

이 문서는 `AIrisDemoGameMode::ApplyScenarioARoleBasedFiltering` 함수의 구현을 바탕으로, Unreal Engine 5(5.7.1 기준)의 새로운 네트워킹 시스템인 **Iris Replication**의 핵심 개념과 동작 방식을 설명합니다. 처음 Iris를 접하는 분들도 이해하기 쉽도록 상세한 호출 흐름과 내부 동작을 중점적으로 다루었습니다.

## 1. 개요 (Overview)

`ApplyScenarioARoleBasedFiltering` 함수는 Scenario A에서 플레이어의 역할(Commander, FieldAgent, Spectator)과 담당 구역(Zone)에 따라 보여야 할 액터들을 선별적으로 복제(Replicate)하는 기능을 담당합니다.
기존 언리얼 엔진의 `AActor::bAlwaysRelevant`나 `NetCullDistance` 등의 속성에 전적으로 의존하지 않고, **Iris의 그룹(Group)과 필터링(Filtering) 시스템**을 사용하여 효율적으로 서버-클라이언트 간의 네트워크 트래픽을 제어합니다.

## 2. 주요 개념 (Iris Core Concepts)

코드를 분석하기 전에 등장하는 Iris의 주요 클래스와 개념을 먼저 이해해야 합니다.

- **`UReplicationSystem`**: Iris 복제 시스템의 심장입니다. 모든 복제 대상 객체, 그룹, 필터링 정책을 총괄하여 관리합니다.
- **`UObjectReplicationBridge`**: 기존 언리얼 엔진의 생태계(`UObject` 및 `AActor`)와 새로운 Iris 시스템을 연결하는 다리 역할을 합니다.
- **`FNetRefHandle`**: Iris 내부에서 복제 객체를 식별하기 위해 사용하는 고유 핸들입니다. `UObject` 포인터를 직접 참조하는 대신 이 핸들을 사용하여 매우 빠르고 가볍게 객체를 추적하고 다룹니다.
- **`FNetObjectGroupHandle`**: 여러 복제 객체를 묶어서 관리하기 위한 그룹 핸들입니다. 필터링 정책을 객체별로 적용하지 않고 일괄 적용할 때 유용합니다.

---

## 3. 호출 흐름 및 상세 분석 (Call Flow & Detailed Analysis)

`ApplyScenarioARoleBasedFiltering` 함수의 실행 흐름은 크게 4단계로 나눌 수 있습니다.

### 단계 1: Iris 시스템 획득 (Initialization & System Acquisition)

가장 먼저 현재 월드의 Iris 시스템을 찾아서 가져와야 합니다.

```cpp
// 1. 현재 모드가 Iris를 사용 중인지 확인
if (!IsIrisReplicationEnabled()) { return; }

// 2. Replication System 가져오기
UReplicationSystem* ReplicationSystem = UE::Net::FReplicationSystemUtil::GetReplicationSystem(World);

// 3. Replication Bridge 가져오기
UObjectReplicationBridge* ReplicationBridge = ReplicationSystem->GetReplicationBridge();
```

**[내부 호출 및 기능 설명]**
- `UE::Net::FReplicationSystemUtil::GetReplicationSystem(World)`: 현재 월드에 연결된 NetDriver 인스턴스에서 작동 중인 `UReplicationSystem`을 가져옵니다.
- `GetReplicationBridge()`: 일반 언리얼 객체(`UObject`)와 상호작용하기 위해 필요한 브릿지를 시스템으로부터 가져옵니다. Iris는 엔진 내부 깊숙이 설계되었기 때문에 객체를 직접 다루지 않고 이 브릿지를 통해 정보를 교환합니다.

### 단계 2: 필터링 그룹 생성 및 기본 상태 설정 (Group Creation)

역할별/구역별로 액터들을 분류해서 담아둘 그룹들을 만듭니다. 데모 코드에서는 `GetOrCreateScenarioAExclusionGroup` 헬퍼 함수 안에서 이 과정이 수행됩니다.

```cpp
// GetOrCreateScenarioAExclusionGroup 내부 흐름:
UE::Net::FNetObjectGroupHandle GroupHandle = ReplicationSystem->FindGroup(GroupName);
if (!ReplicationSystem->IsValidGroup(GroupHandle))
{
    // 그룹을 새로 생성합니다.
    GroupHandle = ReplicationSystem->CreateGroup(GroupName);
    
    // 이 그룹을 Exclusion 필터 모드로 등록합니다. (중요)
    ReplicationSystem->AddExclusionFilterGroup(GroupHandle);
    
    // 그룹의 기본 상태를 Disallow(차단)로 설정합니다.
    ReplicationSystem->SetGroupFilterStatus(GroupHandle, UE::Net::ENetFilterStatus::Disallow);
}
```

**[내부 호출 및 기능 설명]**
- `CreateGroup()`: 명시한 이름(FName)으로 새로운 그룹을 만들고 핸들을 반환받습니다.
- **`AddExclusionFilterGroup()` (핵심!)**: 방금 만든 그룹을 단순히 하나로 묶는 용도가 아니라 'Exclusion Filter(배제 필터)' 정책을 따르도록 등록합니다. 이 필터에 속한 그룹은 클라이언트들에게 복제되는 과정에서 명시적으로 제외될 수 있습니다.
- `SetGroupFilterStatus(..., Disallow)`: 그룹의 글로벌(Global) 필터 상태를 `Disallow`(복제 허용 안 함)로 못 박습니다. 즉, 이 그룹 안에 배치되는 모든 액터는 **기본적으로 그 어떤 클라이언트에게도 전송되지 않습니다.** 

### 단계 3: 그룹에 액터 등록 (Adding Actors to Groups)

월드에 배치되어 있는 센서, 드론, 보급품 액터들을 가져와서 각자의 소속 구역(ZoneId)에 맞는 필터링 그룹에 추가합니다. 데모에서는 `AddActorToScenarioAFilterGroup` 헬퍼 함수가 쓰입니다.

```cpp
// AddActorToScenarioAFilterGroup 내부 흐름:
// 1. 일반 AActor 포인터를 Iris 전용 FNetRefHandle로 변환합니다.
const UE::Net::FNetRefHandle RefHandle = ReplicationBridge->GetReplicatedRefHandle(Actor);

// 2. 유효한 핸들이라면 그룹에 집어넣습니다.
if (RefHandle.IsValid())
{
    ReplicationSystem->AddToGroup(GroupHandle, RefHandle);
}
```

**[내부 호출 및 기능 설명]**
- `GetReplicatedRefHandle(Actor)`: 브릿지를 이용해 언리얼의 `AActor` 포인터를 넘겨주고, Iris 내부에서 사용하는 가벼운 번호표인 `FNetRefHandle`을 받아옵니다. 해당 액터가 네트워크 복제용으로 초기화되어 있어야만 올바른 핸들이 반환됩니다.
- `AddToGroup()`: 핸들을 이용해 대상 그룹에 객체를 추가합니다. 이제부터 해당 액터의 복제 여부는 본인 스스로의 상태가 아닌, 소속된 그룹의 필터링 규칙에 완전히 종속됩니다.

### 단계 4: 클라이언트별 맞춤형 필터링 정책 적용 (Per-Connection Filter Setup)

마지막으로 접속한 모든 플레이어(Connection)를 순회하면서, 그 플레이어의 역할과 담당 구역을 확인하고 어떤 정보를 열람할 수 있을지(어떤 그룹을 Allow 할지) 개별적으로 결정해 줍니다.

```cpp
// 접속 중인 플레이어 컨트롤러 순회 중...
const uint32 ConnectionId = NetConnection->GetConnectionHandle().GetParentConnectionId();

for (int32 ZoneId = 0; ZoneId < ScenarioAZoneCount; ++ZoneId)
{
    // 현재 플레이어의 역할과 구역 정보를 토대로 해당 Zone 데이터 수신 여부 판단
    const UE::Net::ENetFilterStatus DetailStatus = ShouldScenarioAConnectionReceiveDetailZone(OperatorRole, AssignedZoneId, ZoneId) 
        ? UE::Net::ENetFilterStatus::Allow 
        : UE::Net::ENetFilterStatus::Disallow;
        
    // 이 특정 접속자(ConnectionId)에게만 그룹의 필터 상태를 Allow 또는 Disallow로 덮어씁니다.
    ReplicationSystem->SetGroupFilterStatus(DetailZoneGroups[ZoneId], ConnectionId, DetailStatus);
}
```

**[내부 호출 및 기능 설명]**
- `GetParentConnectionId()`: 해당 클라이언트의 물리적인 네트워크 연결 상태를 나타내는 고유 Connection ID를 가져옵니다. **Iris의 필터링 결정은 플레이어 액터 단위가 아닌 이 네트워크 커넥션 단위로 작동합니다.**
- `SetGroupFilterStatus(GroupHandle, ConnectionId, Status)`: 2단계에서 그룹의 상태를 글로벌 `Disallow`로 만들었던 것을 기억하시나요? 이 함수는 **이 특정 클라이언트(ConnectionId)에게만 한정하여 그룹을 `Allow` 상태로 열어줍니다.**
  - 예: FieldAgent 역할의 유저가 Zone 1에 배정되었다면, Zone 1 그룹은 해당 유저의 커넥션에 한해 `Allow`로 덮어씌워져 복제 데이터가 전송됩니다. 나머지 Zone 0과 Zone 2는 `Disallow`인 상태가 유지되어 불필요한 트래픽 낭비가 사라집니다.

---

## 4. 핵심 학습 포인트 정리 (Key Takeaways)

1. **UObject 직접 참조 최소화**: Iris는 필터링을 검사할 때마다 무거운 `AActor` 객체에 접근하거나 형변환을 하지 않습니다. 대신 초기에 Bridge를 통해 `FNetRefHandle`을 얻어두고, 내부에서는 메모리 상 캐시 친화적인 핸들과 ID 배열만을 조작하므로 속도가 대폭 향상되었습니다.
2. **Opt-in 방식의 그룹 필터링 (명시적 허용)**: 기존의 각 액터가 `bAlwaysRelevant` 등에 의지해 본인 스스로가 복제될지 말지를 결정하는 분산된 방식과 다릅니다.
   - 액터들을 목적에 맞게 **그룹으로 묶음**
   - 그룹 전체의 기본 복제 상태를 일괄 차단 (`AddExclusionFilterGroup` + `Disallow`)
   - 정책을 통과한 **특정 커넥션(플레이어)에게만 명시적으로 복제를 허용** (`SetGroupFilterStatus` with `ConnectionId`)
   하는 형태의 **서버 주도의 하향식(Top-Down) 중앙 집중형 필터링**을 사용합니다.
3. **압도적인 연산 효율성**: 액터가 10,000개이고 접속자가 100명이라면, 기존 방식은 매 프레임마다 액터 하나하나가 100명의 클라이언트와 거리를 재거나 가시성 검사를 해야 했습니다. 하지만 Iris의 배제 그룹 필터링을 사용하면 플레이어의 소속 구역이 바뀔 때 단 한 번만 그룹 필터 상태 플래그를 조작해 주면 끝납니다. 엔진 내부 필터링 검사는 그룹 레벨에서 쳐내기 때문에 매 틱 연산량이 극적으로 감소합니다.
