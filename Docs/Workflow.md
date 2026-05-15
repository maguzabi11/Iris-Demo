# IrisDemo 반복 작업 절차

이 문서는 Codex가 IrisDemo 작업을 진행할 때 반복해서 확인하는 절차와 명령을 짧게 고정한다.

## 기본 루프

1. `git status --short`로 사용자 변경과 작업 중 변경을 구분한다.
2. 관련 문서와 코드만 읽고, 변경 범위를 작은 빌드 가능 단위로 잡는다.
3. 코드 변경 후 관련 문서를 같은 작업 단위에서 갱신한다.
4. 가능하면 UE 5.7.1 `IrisDemoEditor Win64 Development` 빌드를 실행한다.
5. 빌드가 성공하면 이번 작업과 관련된 파일만 커밋한다.

## 에디터 빌드

반복 빌드는 루트에서 다음 배치를 사용한다.

```bat
Tools\BuildEditor.bat
```

이 배치는 다음 빌드와 같다.

```bat
G:\UE\UnrealEngine-5.7.1-release\Engine\Build\BatchFiles\Build.bat IrisDemoEditor Win64 Development -Project=G:\UE\Projs\IrisDemo\IrisDemo.uproject -WaitMutex -NoHotReload
```

다른 위치의 UE 5.7.1을 써야 하면 `UE_571_ENGINE_DIR` 환경변수를 `Engine` 디렉터리로 지정한다.

```bat
set UE_571_ENGINE_DIR=D:\UE\UnrealEngine-5.7.1-release\Engine
Tools\BuildEditor.bat
```

추가 UBT 인자가 필요하면 그대로 뒤에 붙인다.

```bat
Tools\BuildEditor.bat -Verbose
```

검증 메모:

- 2026-05-15: `Tools\BuildEditor.bat` 실행 결과 `IrisDemoEditor Win64 Development` 빌드 성공. UBT 결과는 `Target is up to date`.

## Scenario A Baseline 기록

A3 baseline은 filtering 전 control run이다. 실제 측정은 [ScenarioA_RoleBasedFiltering.md](./ScenarioA_RoleBasedFiltering.md)의 A3 절차를 기준으로 한다.

공통 실행 조건:

```text
-ScenarioASeed=1001 -ScenarioASensorCount=6 -ScenarioADroneCount=3 -ScenarioASupplyCrateCount=3 -ScenarioAUpdateInterval=2 -ScenarioARunDuration=30
```

Generic/Iris 비교는 같은 조건에서 `-UseIrisReplication=0`, `-UseIrisReplication=1`만 바꾼다. 각 local client에서는 console에서 `IrisRelayLogBaselineSnapshot`을 실행해 수신 actor count를 로그로 남긴다.
