# 포트폴리오 성능 측정/디버깅 보강 계획

이 문서는 `Iris Relay Lab`을 단순 기능 데모가 아니라, 성능 측정과 디버깅 역량까지 보여주는 포트폴리오 산출물로 만들기 위한 기준 문서다.

## 1. 목표 포지셔닝

이 데모의 핵심 메시지는 “UE Iris를 써봤다”가 아니다. 목표 메시지는 다음과 같다.

> UE 5.7.1의 Iris를 이용해 connection별 관심 데이터 복제를 설계하고, Generic/Iris 조건을 동일하게 맞춘 뒤, 로그/프로파일링/NetTrace 기반으로 결과를 검증한 네트워크 복제 데모.

고난도 회사에서 평가할 가능성이 높은 지점은 기능 구현 자체보다 다음 역량이다.

- 가설을 세우고 통제 조건을 고정하는 능력
- Unreal 네트워크 복제 경로를 코드와 로그로 추적하는 능력
- PIE, command line, ini, dedicated server 차이를 다루는 환경 구성 능력
- Unreal Insights, CSV Profiling, NetTrace 같은 도구로 수치를 남기는 능력
- 결과가 기대와 다를 때 원인을 좁혀 가는 디버깅 능력

## 2. 현재 강점

현재 프로젝트는 이미 튜토리얼 수준을 넘어서는 기반을 갖고 있다.

- Generic/Iris 실행 스위치가 있다.
- 프로젝트 기본 replication mode를 Iris로 고정하고, Generic은 비교용 control run으로 명시 실행하는 기준이 있다.
- role별 replication 결과가 실제로 다르게 나온다.
- Commander/FieldAgent/Spectator의 actor 수신 차이를 로그로 기록했다.
- UE 5.7.1 설정 제약, `TargetRules.bUseIris` 미존재, `SetupIrisSupport(Target)` 유지 같은 환경 차이를 문서화했다.
- Scenario A가 “Replication Graph 없이 Iris Filtering으로 interest control”이라는 명확한 문제를 가진다.

## 3. 부족한 부분

현재 부족한 부분은 기능보다 측정 체계다.

- actor count는 있지만 bandwidth, replicated object count, relevant object count가 아직 없다.
- dedicated server + 3 clients의 Generic/Iris actor count 검증은 1차로 끝났지만, 성능 수치까지 포함한 반복 run은 아직 부족하다.
- 실행 환경은 RunId와 기본 옵션을 저장하기 시작했지만, 프로파일링 산출물까지 완전히 묶인 형태는 아직 아니다.
- Unreal Insights / NetTrace / CSV Profiling 결과가 아직 산출물로 남지 않았다.
- 문제가 생겼을 때 어떻게 추적했는지를 보여주는 디버깅 사례가 부족하다.

따라서 기존 “Scenario A 기능 구현 70~75%” 평가는 맞지만, 성능/디버깅 중심 포트폴리오 완성도는 대략 45~50% 지점으로 보는 편이 현실적이다.

## 4. 보강 목표 축

### 4.1 Replication 설계 능력

- Role, Zone, Detail/Summary metadata 기반 filtering
- UObject/subobject replication
- Prioritization
- Push model dirty marking
- Seamless travel 상태 유지

### 4.2 성능 측정 능력

- Generic vs Iris 비교
- 동일 seed, 동일 duration, 동일 actor count
- bandwidth, replicated object count, server frame time, NetBroadcastTickTime 기록
- Unreal Insights / CSV / NetTrace 결과 보관

### 4.3 디버깅과 환경 구성 능력

- UE 5.7.1 Iris 활성화 조건 정리
- PIE URL option, command line, ini override 차이 정리
- dedicated server 실행법 정리
- 실패 로그와 원인 분석 기록

## 5. 단계별 로드맵

### Phase 1. Scenario A를 재현 가능한 샘플로 마감

- dedicated server + 3 clients 검증
- Generic/Iris 결과 재기록
- 실행 옵션, ini 설정, 서버 형태를 문서에 고정
- false positive/false negative 복제 여부 기록
- “왜 FieldAgent는 4개, Spectator는 0개 detail을 받는가”를 코드 기준으로 설명

### Phase 2. 측정 하네스 추가

- run id 생성
- seed, mode, role filtering on/off, duration, actor count 기록
- 로그를 파일 기반 csv/json으로 저장
- `IrisRelayLogBaselineSnapshot`을 수동 콘솔 명령에서 자동 기록으로 확장
- 결과 표를 수동 복사보다 파일 기반으로 남기는 방향으로 개선

2026-05-22 기준 1차 측정 하네스:

- 서버는 `Saved/ScenarioA/Runs/<RunId>/run.json`에 실행 조건을 저장한다.
- `IrisRelayLogBaselineSnapshot`은 `Saved/ScenarioA/Runs/<RunId>/client_snapshots.csv`에 local client 수신 결과를 append한다.
- local 실행 배치는 `ScenarioA_yyyyMMdd_HHmmss` 형식의 RunId를 서버와 client에 공통 전달한다.
- 다음 보강은 actor count 외의 `outgoing bandwidth`, replicated object count, CSV/NetTrace 중 하나를 같은 RunId 아래에 남기는 것이다.

2026-05-26 기준 실행 모드 기준:

- IrisDemo의 기본 replication mode는 Iris다 (`DefaultEngine.ini`, local run batch 모두 `UseIrisReplication=1`).
- Generic은 기본값이 아니라 기존 replication path와 비교하기 위한 control run이며, `--no-iris` 또는 `-UseIrisReplication=0`으로 명시 실행한다.
- 따라서 Generic/Iris 비교 문서에서는 “기본값이 무엇인가”보다 “동일 조건에서 replication mode override만 바꿨는가”를 검증 기준으로 둔다.

### Phase 3. Unreal Insights / NetTrace / CSV 수집

비교 run은 최소 다음 네 가지를 둔다.

- Generic baseline
- Iris filtering off
- Iris filtering on
- Iris filtering on + network emulation

각 run에서 최소한 다음 값을 남긴다.

- server frame time
- NetBroadcastTickTime
- outgoing bandwidth
- replicated object count
- relevant object count per connection
- packet size

### Phase 4. Scenario C 또는 B 확장

추천 우선순위는 Scenario C다.

`UObject/Subobject Replication`은 Actor replication과 connection filtering 다음 단계의 설계 능력을 보여주기 좋다. Actor wrapper 없이 item/loadout/cargo lifetime을 다루면 20년 이상 경력 프로그래머의 설계 경험을 더 잘 드러낼 수 있다.

그 다음 Scenario B의 prioritization/push model을 붙이면 성능 측정과 연결하기 쉽다.

### Phase 5. 제출용 문서 정리

최종 포트폴리오에는 코드 외에 다음 문서가 필요하다.

- `ScenarioA_RoleBasedFiltering.md`: 기능, 정책, 결과
- `PerformanceRuns.md`: 측정 조건과 결과 표
- `DebuggingNotes.md`: 설정 문제, 실패 로그, 해결 과정
- `Iris기본지식.md`: UE 5.7.1 기준 학습/검증 사실
- 짧은 README: 문제 정의, 재현법, Generic/Iris 결과 차이, 성능 측정 방법

## 6. 면접에서 강조할 이야기

- Iris가 빠르다고 가정하지 않고 Generic과 같은 조건으로 비교했다.
- role은 actor 속성이 아니라 connection/player state 속성으로 다뤘다.
- actor metadata와 connection role을 분리해서 filtering policy를 만들었다.
- PIE option, ini, command line 차이를 확인하고 재현 가능한 실행 경로를 문서화했다.
- 기능 결과뿐 아니라 false positive 복제 여부와 측정 조건을 남겼다.
- 다음 단계로 bandwidth/object count/NetTrace까지 붙일 수 있는 구조로 만들었다.

## 7. 다음 우선순위

1. Unreal Insights / CSV / NetTrace 최소 수집 경로 정리
2. outgoing bandwidth 또는 replicated object count 중 최소 1개 기록
3. `PerformanceRuns.md`와 `DebuggingNotes.md` 생성
4. Scenario C UObject/subobject replication 최소 구현 착수
