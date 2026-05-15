# IrisDemo 작업 규칙

## 사용자와의 대화
사용자와 대화 시 태도와 말투는 공손하고 친절하게, 존댓말로 통일.
문서나 혼자말은 존댓말을 안써도 상관 없다.

## 프로젝트 기준

- 엔진 기준은 UE 5.7.1이다.
- 프로젝트명과 모듈명은 `IrisDemo`로 유지한다. `IrisDemo_UE` 이름을 다시 도입하지 않는다.
- 데모 방향은 `Docs/planning.md`의 `Iris Relay Lab` 기획을 기준으로 한다.

## Iris 설정

- Iris 활성화는 `.uproject`, `IrisDemo.Build.cs`, `DefaultEngine.ini`, 실행 인자로 관리한다.
- UE 5.7.1 기준 `TargetRules.bUseIris`는 존재하지 않는다. Target.cs에 `bUseIris = true;`를 추가하지 않는다.
- `Source/IrisDemo/IrisDemo.Build.cs`의 `SetupIrisSupport(Target)`를 유지한다.
- 모듈 export macro는 `IRISDEMO_API`를 사용한다.

## Git/파일 관리

- `Docs/`, `Config/`, `Content/`, `Source/`, `IrisDemo.uproject`는 추적 대상이다.
- `Binaries/`, `Intermediate/`, `Saved/`, `.firecrawl/`는 커밋하지 않는다.
- UE 에디터가 만든 대량 변경은 작업 목적과 관련 있는지 확인한 뒤 커밋한다.

## 개발 순서

- 큰 기능을 한 번에 넣지 말고, 빌드 가능한 작은 단위로 구현하고 커밋한다.
- 사용자가 확인할 수 있는 분량으로 작업을 끊어서 진행한다.
- 다음 작업으로 넘어가기 전에 이전 작업의 완료 여부와 검증 로그를 사용자에게 확인받는다.
- 변경 후 가능하면 UE 5.7.1로 `IrisDemoEditor Win64 Development` 빌드를 확인한다.
- 빌드 확인은 사용자에게 다시 묻지 않고 바로 실행한다.
- 빌드가 성공하면 별도 확인 없이 관련 변경을 즉시 커밋한다.
- 먼저 Scenario A의 role-based filtering 골격을 만들고, 이후 UObject subobject replication과 seamless travel 검증으로 확장한다.
- Scenario A에서 Commander/FieldAgent/Spectator 같은 role은 테스트 actor 속성이 아니라 player/connection 속성으로 다룬다.
- 테스트 actor에는 `ZoneId`, category, owner, summary/detail 구분 같은 metadata를 두고, filtering policy가 player role과 actor metadata를 비교해 복제 여부를 결정한다.

## 문서

- 작업 후에는 관련 문서를 항상 갱신한다.
- Iris 관련 사실은 실제 UE 5.7.1 빌드 결과와 Epic 공식 문서를 우선한다.
- 새로 알게 된 제약이나 설정 차이는 `Docs/planning.md` 또는 `Docs/Iris기본지식.md`에 반영한다.
