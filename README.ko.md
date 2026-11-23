# Cloppy

Windows용 클립보드 매니저. 복사한 모든 내용을 모으고, 코드 블록·에셋·폴더로 정리하고, 빠른 창에서 바로 붙여넣습니다. C++ / Qt 6 / Qt Quick로 만들었습니다.

## 주요 기능

- **클립보드 기록**: 텍스트, 링크, 이미지, 파일을 자동 저장. 중복 병합, 고정(📌), 즐겨찾기, 시크릿 모드, 자동 정리
- **순서 유지**: 같은 내용을 다시 복사하거나 붙여넣어도 목록 순서가 그대로 유지됩니다(설정 → 동작). 트레이 메뉴·빠른 창·메인 창에서 ‘전체 지우기 (고정 제외)’로 고정하지 않은 클립을 한 번에 정리
- **빠른 창 (Win+V)**: Windows 기본 클립보드 대신 Cloppy 패널이 열립니다. 클립 / 코드 / 에셋 전환, 폴더 필터, 검색, 키보드 탐색, 창 크기 조절(기억됨)
- **코드 블록**: 제목·언어·폴더로 코드 조각 보관, 자동 삭제에서 보호
- **에셋**: 이미지·폰트·코드 파일을 앱 폴더로 복사해 보관, 미리보기/열기/경로 복사
- **폴더**: 클립·에셋 폴더와 하위 폴더, 색상, 이동
- **텍스트 확장**: `!mail` → `me@example.com` 같은 단축어. 한글 단축어(`!메일`) 지원, Cloppy 창 안에서도 동작
- **한/영 변환**: `Ctrl+Alt+L` 선택 영역 즉시 변환, `Win+Alt+L` 클립보드 변환
- **개발자 모드**: 터미널 복사 정리(ANSI 제거), 터미널 붙여넣기 단축키, 서식 없이 붙여넣기
- **변환하여 붙여넣기 (개발자 도구)**: JSON 압축/정리/문자열, HTML 줄바꿈(`<br>`)·엔티티 변환/해제, 문자열 이스케이프/해제, URL 인코딩/디코딩, Base64 인코딩/디코딩, Markdown 코드블록/인라인, 한 줄로·쉼표 연결·공백 정리, `camelCase`/`snake_case`/`kebab-case`/`CONSTANT_CASE`, 각 줄 `//`·`#` 주석
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

결과: `dist\` 폴더와 `Cloppy-portable.zip`. `dist\Cloppy.exe`를 실행하며, 폴더 전체가 필요합니다.
배포에 필요한 라이선스 파일(`LICENSE`, `LICENSES\`)과 문서가 함께 들어갑니다.

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
docs/       배포에 포함되는 사용법 문서
LICENSES/   LGPL-3.0 · GPL-3.0 라이선스 전문
installer/  Qt Installer Framework 구성
build.bat / package.bat / run.bat
```

## 라이선스

Cloppy는 **GNU Lesser General Public License v3(또는 그 이상의 버전)**로 배포되는
자유 소프트웨어입니다. 프로젝트 라이선스는 [LICENSE](LICENSE), 라이선스 전문은
[LICENSES](LICENSES)의 `LGPL-3.0.txt`, `GPL-3.0.txt`를 참고하세요.

Cloppy는 **Qt**로 만들어졌으며 Qt는 **GNU LGPL v3** 조건으로 사용합니다.

```
The Qt Toolkit is Copyright (C) 2017 The Qt Company Ltd.
Contact: https://www.qt.io/licensing
```

Qt는 DLL로 동적 연결되어 있으며, `Cloppy.exe` 옆의 Qt DLL과 QML 모듈은 누구나
교체·재연결할 수 있습니다. 포터블 zip과 설치 프로그램 모두 `LICENSES` 폴더에
라이선스 전문을 포함하고, 앱 안에서도 트레이 메뉴 → 라이선스로 확인할 수 있습니다.
Qt 소스는 <https://download.qt.io/official_releases/qt/> 와 <https://code.qt.io/> 에서
얻을 수 있습니다.

함께 배포되는 서드파티 구성 요소(Qt, SQLite, Microsoft VC++ 런타임, Qt Installer
Framework)와 조건은 [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)에 정리해 두었습니다.
