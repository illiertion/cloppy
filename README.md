# Cloppy

Windows용 클립보드 매니저. 복사한 모든 내용을 모으고, 코드 블록·에셋·폴더로 정리하고, 빠른 창에서 바로 붙여넣습니다. C++ / Qt 6 / Qt Quick로 만들었습니다.

## 주요 기능

- **클립보드 기록**: 텍스트, 링크, 이미지, 파일을 자동 저장. 중복 병합, 고정(📌), 즐겨찾기, 시크릿 모드, 자동 정리
- **빠른 창 (Win+V)**: Windows 기본 클립보드 대신 Cloppy 패널이 열립니다. 클립 / 코드 / 에셋 전환, 폴더 필터, 검색, 키보드 탐색, 창 크기 조절(기억됨)
- **코드 블록**: 제목·언어·폴더로 코드 조각 보관, 자동 삭제에서 보호
- **에셋**: 이미지·폰트·코드 파일을 앱 폴더로 복사해 보관, 미리보기/열기/경로 복사
- **폴더**: 클립·에셋 폴더와 하위 폴더, 색상, 이동
- **텍스트 확장**: `!mail` → `me@example.com` 같은 단축어. 한글 단축어(`!메일`) 지원, Cloppy 창 안에서도 동작
- **한/영 변환**: `Ctrl+Alt+L` 선택 영역 즉시 변환, `Win+Alt+L` 클립보드 변환
- **개발자 모드**: 터미널 복사 정리(ANSI 제거), 터미널 붙여넣기 단축키, 서식 없이 붙여넣기
- **변환하여 붙여넣기**: JSON 압축/정리, HTML 줄바꿈, Markdown 코드블록, 한 줄로, 따옴표, Base64
- **영어/한국어 UI**: 기본은 English, 설정 → 모양 → 언어에서 한국어로 전환 (즉시 적용)
- **앱 내 사용법**: 사이드바 `? 사용법`, `F1`, 트레이 메뉴

## 요구 사항

- Windows 10/11 64비트
- Qt 6.5 이상 (개발 환경: Qt 6.12.0 `msvc2022_64`)
- Visual Studio 2022 이상 (MSVC, CMake, Ninja 포함)
- Qt Installer Framework 4.x (설치 프로그램을 만들 때만)

## 빌드

`build.bat`의 `QT_DIR`, `VS_DIR` 경로를 본인 환경에 맞게 고친 뒤 실행합니다.

```bat
build.bat
```

결과: `build\Cloppy.exe` (windeployqt로 필요한 DLL과 QML 모듈이 함께 배치됩니다)

## 포터블 패키지

```bat
package.bat
```

결과: `dist\` 폴더. `dist\Cloppy.exe`를 실행하며, 폴더 전체가 필요합니다.

## 설치 프로그램 (Qt Installer Framework)

```bat
package.bat
installer\build-installer.bat
```

결과: `CloppySetup.exe`. 설치 시 시작 메뉴와 바탕화면 바로가기를 만들고, 기본 경로는 `%USERPROFILE%\Cloppy`입니다.
IFW 경로가 다르면 `installer\build-installer.bat`의 `IFW_DIR`을 수정하세요.

## 단축키

| 단축키 | 동작 |
| --- | --- |
| `Win+V` | 빠른 창 열기/닫기 |
| `Ctrl+,` / `F1` | 설정 / 사용법 |
| `Ctrl+Alt+L` | 선택한 텍스트 한/영 변환 |
| `Win+Alt+L` | 클립보드 내용 한/영 변환 |
| `Ctrl+1 / 2 / 3` | 클립 / 코드 블록 / 에셋 보기 |
| `Ctrl+F`, `Ctrl+N`, `Ctrl+Shift+N` | 검색, 새 폴더, 새 코드 블록 |
| `Enter` / `Del` | 복사 / 삭제 |

모든 전역 단축키는 설정에서 바꾸거나 해제할 수 있습니다.

## 데이터 위치

```
%APPDATA%\Cloppy\Cloppy\cloppy.db      클립·폴더·설정 DB (SQLite)
%APPDATA%\Cloppy\Cloppy\images\        이미지 클립
%APPDATA%\Cloppy\Cloppy\assets\        에셋 파일
%APPDATA%\Cloppy\Cloppy\cloppy.log     로그
```

## 언어

UI 기본 언어는 English이며, 설정 → 모양 → Language에서 한국어로 바꿀 수 있습니다. 언어는 QSettings(`language`)에 저장되고, QML은 `Lang.t(한국어, English)` 싱글턴으로, C++ 알림은 `Tr::text()`로 번역합니다.

## 구조

```
src/        C++ (AppController, ClipStore, HotkeyManager, WinUtils, LangSwap ...)
qml/        Qt Quick UI (MainWindow, PopupWindow, SettingsWindow, HelpWindow ...)
assets/     아이콘
installer/  Qt Installer Framework 구성
build.bat / package.bat / run.bat
```
