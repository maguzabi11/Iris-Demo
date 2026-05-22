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

## 로컬 멀티플레이 테스트 실행

테스트 편의를 위해 전용 서버 1개와 클라이언트 3개를 동시에 실행하는 배치 스크립트를 제공한다. 이 스크립트는 uncooked 에디터 바이너리를 통해 세션을 시작하므로 별도의 패키징 없이 즉시 로컬에서 테스트할 수 있다.

```bat
Tools\RunLocalMultiplay.bat [--no-iris] [추가인자]
```

기본적으로 Iris 복제가 활성화되며 (`-UseIrisReplication=1`), `--no-iris` 옵션을 제공하면 Iris 복제를 비활성화 (`-UseIrisReplication=0`)한 상태로 실행한다. 그 외에 지정된 인자(예: `-ScenarioARunDuration=30`)는 서버와 모든 클라이언트에 그대로 전달된다.

빌드된 `Binaries\Win64\IrisDemoServer.exe`와 `Binaries\Win64\IrisDemoClient.exe`로 같은 구성을 실행하려면 다음 배치를 사용한다.

```bat
Tools\RunLocalBinariesMultiplay.bat [--no-iris] [추가인자]
```

이 배치는 실행 전에 서버/클라이언트 exe 존재 여부를 확인한다. 바이너리가 없으면 먼저 `Tools\BuildServer.bat`와 `Tools\BuildClient.bat`로 Development Server/Client 타깃을 빌드한다. 서버 실행 후에는 기본 60초 동안 UDP 게임 포트가 바인딩될 때까지 기다린 뒤 클라이언트를 실행한다. 대기 시간을 바꾸려면 `--server-wait-timeout 30`처럼 지정하고, 창을 띄우지 않고 경로와 최종 인자만 확인하려면 `--dry-run`을 붙인다.

## Scenario A Baseline 기록

A3 baseline은 filtering 전 control run이다. 실제 측정은 [ScenarioA_RoleBasedFiltering.md](./ScenarioA_RoleBasedFiltering.md)의 A3 절차를 기준으로 한다.

공통 실행 조건:

```text
-ScenarioASeed=1001 -ScenarioASensorCount=6 -ScenarioADroneCount=3 -ScenarioASupplyCrateCount=3 -ScenarioAUpdateInterval=2 -ScenarioARunDuration=30
```

Generic/Iris 비교는 같은 조건에서 `-UseIrisReplication=0`, `-UseIrisReplication=1`만 바꾼다. 각 local client에서는 console에서 `IrisRelayLogBaselineSnapshot`을 실행해 수신 actor count를 로그로 남긴다.
