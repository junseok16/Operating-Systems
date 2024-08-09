![header](https://capsule-render.vercel.app/api?type=waving&color=gradient&height=280&section=header&text=Operating%20Systems&fontSize=70&fontColor=ffffff&fontAlign=50&fontAlignY=45)

## 🖥️ 운영체제 프로젝트
이 프로젝트는 `리눅스 셸`과 `멀티스레드 프로그래밍`을 경험하고 `프로세스 스케줄링`, `가상 메모리`, 그리고 `파일 시스템`을 C로 시뮬레이션하는 다섯 가지 프로그램을 개발합니다. 사용자 영역과 커널 영역을 이해하고 리눅스 명령어를 처리하는 나만의 셸을 만듭니다. 멀티스레드 프로그래밍에 병행성 기법을 도입하여 생산자 소비자 문제를 해결합니다. 싱글 프로세서로 CPU 중심 작업과 IO 중심 작업으로 구분된 프로세스를 여러 가지 알고리즘으로 스케줄링하고 가상 메모리를 도입합니다. 그리고 유닉스 파일 시스템으로 디스크 이미지 파일을 관리하는 파일 시스템을 시뮬레이션합니다.
<br>

## 🔎 프로필
- 이름 `운영체제 프로젝트`
- 기간 `2021-09 ~ 2021-12`
- 인원 `2명`
<br>

## ⚙️ 개발 환경
- 언어 `C`
- 통합개발환경 `Visual Studio 2019`
- 형상관리 `Git`
- 운영체제 `Ubuntu 16.04.7 LTS`
<br>

## 💻 목적
- 사용자 모드와 커널 모드의 차이를 구분하고 프로세스를 관리하는 `시스템 호출`을 학습한다.
- `병행성`이 가져오는 동기화 문제를 이해하고 `상호 배제`를 배운다.
- `CPU 스케줄링`과 관련된 설계 이슈를 분류하고 그 유형을 살펴본다.
- `가상 메모리`의 `요구 페이징 기법`을 분석하고 논리 주소가 물리 주소로 변환되는 매커니즘을 이해한다.
- `파일 시스템`을 학습하고 디스크 공간을 할당해 파일 위치를 추적하는 방법을 터득한다.
<br>

## 📜 구현 사항
#### 1. Simple Shell
- 리눅스 시스템 호출로 `배시(Bash) 셸`을 개발합니다.
- 리눅스 내외부 명령어(cd, history, cat, man, vi, gcc 등)를 처리합니다.
- [Simple Shell 프로젝트 기술서](https://drive.google.com/file/d/1epihrdKyyTJ0bYC_vagie5DCwb-0mO4z/view?usp=sharing)

#### 2. Multi-threaded Word Count
- `멀티스레드`로 텍스트 수를 분석하는 프로그램을 개발합니다.
- `뮤텍스(Mutex)`로 생산자-소비자 문제를 해결합니다.
- CPU 사용량 분석 후 생산자-소비자 수를 최적화합니다.
- [Multi-threaded Word Count 프로젝트 기술서](https://drive.google.com/file/d/1SBJHakvP0SsmQMeTOCGLn8C_9R7s1Wfa/view?usp=sharing)

#### 3. Scheduling Simulation
- 가상 싱글 프로세서로 `프로세스 스케줄링` 시뮬레이터를 개발합니다.
- FIFO, 라운드 로빈 알고리즘으로 `문맥 전환`을 구현합니다.
- [Scheduling Simulation 프로젝트 기술서](https://drive.google.com/file/d/1IPTZ0gxr57jVDPZhyqqHeS2ptWO5yJeF/view?usp=sharing)

#### 4. Multi-process Execution with Virtual Memory
- 프로세스 스케줄링에 `가상 메모리` 시뮬레이터를 개발합니다.
- `2레벨 요구 페이징 기법`을 구현합니다.
- LRU 알고리즘으로 페이지 스와핑 기법을 구현합니다.
- [Multi-process Execution with Virtual Memory 프로젝트 기술서](https://drive.google.com/file/d/1_Mk6czFltS1W0hqcmfhKZiQqs9WYV8bJ/view?usp=sharing)

#### 5. File System Implementation
- 유닉스 `파일 시스템` 시뮬레이터를 개발합니다.
- 파일 연산(open, read, write, close) 시스템을 구현합니다.
- `해시 테이블`로 인덱스 할당 기법을 처리합니다.
- [File System Implementation 프로젝트 기술서](https://drive.google.com/file/d/1_3X9OD5vK1DLCGpLRRh29GWqqdsZMq4E/view?usp=sharing)
<br>

## ⓒ 2021-2024. Tak Junseok all rights reserved.
이 리포지토리에 기재된 코드와 리포트에 대한 저작권은 탁준석과 단국대학교 모바일시스템공학과 유시환 교수에게 있습니다. 사전에 합의되지 않은 내용을 무단으로 도용(URL 연결 등), 불법으로 복사(캡처)하여 사용할 경우 사전 경고 없이 저작권법에 의한 처벌을 받을 수 있습니다.

The copyright for the codes and reports listed in this repository belongs to Tak Junseok and Yoo Sihwan, a professor of Mobile System Engineering at Dankook University. If you steal(such as URL connection) or illegally copy(capture) anything that is not agreed in advance, you may be punished by copyright law without prior warning.
