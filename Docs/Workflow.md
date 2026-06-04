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
Tools\RunLocalMultiplay.bat [--iris|--no-iris] [추가인자]
```

프로젝트 기본 설정은 `Config/DefaultEngine.ini`의 `net.Iris.UseIrisReplication=1`이다. 로컬 실행 배치도 기본 mode option을 `--iris`로 두고 Iris 복제를 명시 활성화하며 (`-UseIrisReplication=1`), `--no-iris` 옵션을 제공하면 비교용 Generic replication run으로 전환한다 (`-UseIrisReplication=0`). 배치는 mode에 따라 role filtering 기본값도 같이 고정한다. `--iris`는 `-ScenarioAEnableRoleFiltering=1`, `--no-iris`는 `-ScenarioAEnableRoleFiltering=0`을 자동 전달한다.

Scenario A의 코드 기본값은 성능 비교용 조건이다. 별도 actor count/window 옵션을 붙이지 않아도 detail actor 120개(sensor 60, drone 30, supply crate 30), `ScenarioANetworkMetricsDuration=60`, `ScenarioAAutoSnapshotDelay=95` 기준으로 기록된다. `ScenarioANetworkMetricsStartDelay`는 dedicated/local binary 성능 비교 run에서는 기본 35초이고, PIE에서는 빠른 확인을 위해 기본 5초다. 명시적으로 `-ScenarioANetworkMetricsStartDelay=<seconds>` 또는 PIE URL option을 넘기면 그 값이 항상 우선한다.

빌드된 `Binaries\Win64\IrisDemoServer.exe`와 `Binaries\Win64\IrisDemoClient.exe`로 같은 구성을 실행하려면 다음 배치를 사용한다.

```bat
Tools\RunLocalBinariesMultiplay.bat [--iris|--no-iris] [추가인자]
```

이 배치는 실행 전에 서버/클라이언트 exe 존재 여부를 확인한다. 바이너리가 없으면 먼저 `Tools\BuildServer.bat`와 `Tools\BuildClient.bat`로 Development Server/Client 타깃을 빌드한다. C++ 측정 기본값이나 CSV schema를 바꾼 뒤에는 exe가 이미 있어도 반드시 두 빌드를 다시 실행한다. Blueprint, map, asset, GameMode default object, cooked content가 바뀌었거나 `Saved\Cooked` 로드 오류가 나면 `Tools\CookClientServer.bat`로 Client/Server cooked content도 다시 만든다. 서버는 PowerShell 래핑 없이 exe를 직접 실행하고, `-log -stdout -FullStdOutLogOutput`으로 서버 로그 창과 UE 로그 출력을 사용한다. 서버 실행 후에는 기본 5초 동안 단순 대기한 뒤 클라이언트를 실행한다. 대기 시간을 바꾸려면 `--server-startup-wait 10`처럼 지정한다. 기존 호환용으로 `--server-wait-timeout 10`도 같은 의미로 받으며, 창을 띄우지 않고 경로와 최종 인자만 확인하려면 `--dry-run`을 붙인다.

바이너리 실행 전 전체 갱신 순서:

```bat
Tools\BuildServer.bat
Tools\BuildClient.bat
Tools\CookClientServer.bat
```

개별 cook만 필요하면 아래처럼 실행한다.

```bat
Tools\CookClient.bat
Tools\CookServer.bat
```

## Scenario A Baseline 기록

A3 baseline은 filtering 전 control run이다. 실제 측정은 [ScenarioA_RoleBasedFiltering.md](./ScenarioA_RoleBasedFiltering.md)의 A3 절차를 기준으로 한다.

공통 실행 조건:

```text
-ScenarioASeed=1001 -ScenarioASensorCount=60 -ScenarioADroneCount=30 -ScenarioASupplyCrateCount=30 -ScenarioAUpdateInterval=2 -ScenarioARunDuration=95 -ScenarioANetworkMetricsStartDelay=35 -ScenarioANetworkMetricsDuration=60 -ScenarioANetworkMetricsInterval=1 -ScenarioAAutoSnapshotDelay=95
```

동일 window 비교 실행 예:

```bat
Tools\RunLocalBinariesMultiplay.bat --no-iris --run-id ScenarioA_GenericHeavy_001
Tools\RunLocalBinariesMultiplay.bat --iris --run-id ScenarioA_IrisHeavy_001
```

Generic/Iris 비교는 같은 조건에서 `-UseIrisReplication=0`, `-UseIrisReplication=1`만 바꾼다. 이 프로젝트의 자연스러운 기본 run은 Iris이며, Generic은 기존 replication path와 비교하기 위해 명시적으로 끄는 control run이다. 각 local client는 기본 95초 뒤 snapshot CSV를 자동 기록한다. 필요할 때는 console에서 `IrisRelayLogBaselineSnapshot`을 실행해 수동 snapshot도 추가로 남길 수 있다.

서버는 같은 RunId 폴더에 `server_network_metrics.csv`를 생성하고, `ScenarioANetworkMetricsInterval` 기본값 기준으로 NetDriver 전체 및 client connection별 outgoing bandwidth를 append한다. `ScenarioANetworkMetricsStartDelay`가 0보다 크면 해당 지연 이후 기록을 시작하고, `ScenarioANetworkMetricsDuration`이 0보다 크면 지정된 시간 뒤 기록을 멈춘다. 35초 지연은 초기 접속, role assignment, 첫 복제 burst를 성능 비교 window에서 빼기 위한 보수값이며, PIE의 빠른 smoke check에는 기본 5초 지연을 사용한다.
