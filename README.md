# DirectX11-Flight

[유튜브 영상](https://youtu.be/LtHkrbhR0IQ?si=OpMynCfWETIsJcHL)

## 개발 개요

- **개발 인원** : 1인 개인 프로젝트  
- **개발 환경** : C++, DirectX 11, HLSL, Visual Studio


## 구현 목록

### 🚀 비행 시스템
- 쿼터니언 기반의 회전 처리
- 카메라와 연동된 부드러운 조작
- 'World of Warplanes' 스타일의 조작 방식 일부 구현

### 🎥 카메라 시스템
- 1인칭 / 3인칭 / 자유 시점 카메라 전환
- 마우스 방향에 따른 시야 회전

### 🎯 무기 및 피격 처리
- 총알 발사 및 충돌 판정
- 탄환 수명 및 풀링 시스템
- 피격 시 폭발 파티클 연출

### 💥 파티클 & 이펙트
- 알파 블렌딩 기반 빌보드 파티클
- 총구 화염, 연기, 폭발 등 다양한 연출 효과
- 스프라이트 시트 기반 애니메이션 처리

### 🎮 게임 시스템
- Skybox 렌더링
- 지형 (Heightmap 기반) 렌더링
- PhysX 연동을 통한 충돌 처리 및 간단한 물리

<br>

## 기타
- 자체 구조의 `GameObject`, `Renderer`, `TextureManager` 등 핵심 모듈 구성
- Depth, Blend, Rasterizer 상태를 상황에 따라 분기 처리
- 코드 리팩토링 및 상태 분리