# IrisDemo 포트폴리오 요약

## 한 줄 요약

UE 5.7.1의 Iris Replication을 이용해 멀티플레이 작전 상황에서 역할별로 필요한 네트워크 데이터만 복제하도록 설계하고, Generic/Iris 조건을 통제해 actor count와 outgoing bandwidth를 비교한 네트워크 복제 데모다.

## 문제 정의

같은 월드에 접속한 플레이어라도 역할에 따라 필요한 정보가 다르다.

| 역할 | 필요한 정보 |
|------|-------------|
| Commander | 전체 센서, 드론, 보급품 상태와 작전 요약 |
| FieldAgent | 자기 구역의 상세 정보 |
| Spectator | 실시간 상세 정보가 아닌 요약 정보 |

이 프로젝트는 “모든 actor를 모두에게 보내는 방식” 대신, player/connection role과 actor metadata를 비교해 connection별 복제 대상을 다르게 만드는 구조를 검증한다.

## 구현 핵심

- 역할은 actor 속성이 아니라 `ARelayPlayerState`의 replicated connection/player state로 다룬다.
- 테스트 actor는 `ZoneId`, category, detail/summary level 같은 metadata를 가진다.
- Scenario A는 Iris group filtering을 사용해 connection별 detail actor 수신 대상을 제어한다.
- Generic replication은 비교용 control run으로 실행하고, Iris filtering run과 같은 seed/count/duration 조건을 맞춘다.
- RunId 기반으로 `run.json`, `client_snapshots.csv`, `server_network_metrics.csv` 산출물을 남겨 유효 run과 무효 run을 구분한다.

## 검증 결과

120 detail actor heavy 조건에서 3회 반복 측정한 결과, Generic은 세 역할 모두 120개 detail actor를 수신했고, Iris filtering은 역할별 수신 actor 수를 의도대로 제한했다.

| 모드 | Commander | FieldAgent | Spectator |
|------|-----------|------------|-----------|
| Generic | detail 120 | detail 120 | detail 120 |
| Iris filtering | detail 120 | detail 40 | detail 0 |

3회 heavy 평균 기준 outgoing bandwidth도 같은 방향으로 감소했다.

| 항목 | Generic 평균 | Iris 평균 | 차이 |
|------|--------------|-----------|------|
| Commander AvgOutBytesPerSecond | 2317.69 | 2100.40 | -9.4% |
| FieldAgent AvgOutBytesPerSecond | 2326.39 | 1752.34 | -24.7% |
| Spectator AvgOutBytesPerSecond | 2311.83 | 1576.34 | -31.8% |
| NetDriver DeltaOutTotalBytes | 416845.00 | 326283.00 | -21.7% |

작은 12 actor 조건에서는 actor count는 기대대로 줄었지만 bandwidth 개선은 확인되지 않았다. 이 결과는 작은 payload에서 고정 overhead와 초기 replication 비용이 더 크게 보일 수 있음을 보여주는 사례로 남겼다.

## 보여주는 역량

- UE 5.7.1 Iris 설정과 Generic/Iris 실행 경로 분리
- role, zone, category 기반 네트워크 interest 설계
- Replication Graph 없이 Iris filtering으로 connection별 관심 대상을 구성
- dedicated server + clients 로컬 비교 실행 구성
- 동일 seed/count/duration 조건의 반복 측정
- 유효 run/무효 run 판정과 stale binary 문제 분석
- UObject registered subobject replication으로 확장 가능한 구조 설계

## 현재 상태와 다음 단계

Scenario A는 actor filtering과 bandwidth 반복 측정까지 마감했다. Scenario C는 `URelayCargoItem`, `ARelayCargoStationActor`, `URelayCargoInventoryComponent` 기반의 UObject/subobject replication 최소 구현과 빌드 검증을 완료했고, item array replication과 role-based policy 결합으로 확장 중이다.

다음 단계는 다음과 같다.

1. Scenario C 다중 cargo item replication을 local binary에서 반복 확인
2. cargo item visibility를 owner/role/zone policy와 결합
3. Scenario B prioritization/push model을 측정 하네스와 연결
4. Scenario D seamless travel에서 PlayerState/loadout 유지 검증
5. Unreal Insights/NetTrace 분석 결과를 문서화

## 참고 문서

- [planning.md](./planning.md)
- [ScenarioA_RoleBasedFiltering.md](./ScenarioA_RoleBasedFiltering.md)
- [PerformanceRuns.md](./PerformanceRuns.md)
- [DebuggingNotes.md](./DebuggingNotes.md)
- [ScenarioC_UObjectSubobjectReplication.md](./ScenarioC_UObjectSubobjectReplication.md)
