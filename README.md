# DirectX12 Renderer

## 개요
- 개발인원 : 개인 프로젝트
- 개발목표 : 그래픽스 학습 및 멀티스레드 렌더링 개념 이해
  
<br>

- [비행기 쉐이딩 발전 과정](https://wandering-rumba-865.notion.site/227aba645d3280afbfbbd014fd699dd9?pvs=74)

<br>

- **구현과정에서는 AI와 인터넷 강의를 많이 활용**하였기에, 아래 문서와 같이 **마주한 문제들을 어떻게 해결해나갔는지를 어필하고 싶었습니다**
- **[DirectX12 구현 중 겪은 문제들과 해결과정](https://wandering-rumba-865.notion.site/DX12-26baba645d328061aeffe7122b04008b?pvs=74)**

<br>

- 멀티스레드 렌더링과 관련된 부분은 Microsoft 공식샘플코드를 그대로 따라 사용한 것이 아닌,
  <br> **C++20 문법의 `std::barrier` 를 활용하여 원리는 동일하지만 구현방식은 바꾸어 구현해보며 멀티스레드 렌더링과  `std::barrier` 개념을 이해**하고자 하였습니다.

<br>

- 또한 RenderPass 관련 부분은, 기존 샘플코드를 클래스로 구조화함으로써 재사용성과 확장성을 높이고자 하였습니다.

<br>

---

## 생성형 AI 및 오픈소스를 활용한 부분
- 클래스 설계와 소스코드 초안을 만들고, DirectX12 API를 사용하는 부분에서 AI를 활용하였습니다.
- 해당 코드를 점검하고 수정해가며 DirectX12 를 공부해보자는 마인드로 시작하였습니다.

<br>
  
- **기본적인 코드베이스는 Microsoft 공식샘플코드와 Frank Luna 샘플코드를 기반**으로 하였습니다.

<br>
  
- 해당 문서처럼 [DirectX12 멀티스레드 오픈소스 살펴보기](https://wandering-rumba-865.notion.site/1-242aba645d328051acfac15632e76cf5?pvs=7), 오픈소스를 분석하고 이해하고자 하였습니다

<br>

## 참고 자료
- [유영천님 DirectX 12 프로그래밍 소개 강의](https://www.youtube.com/live/Z9veGJv7nPE?si=kNchvTLwyCxX-FnF) 와 Frank Luna 서적을 보면서 공부를 하였습니다.

<br>

- **PBR**, **ShadowMap**, **PostProcess** 는
  [홍정모 교수님 DirectX 11 강의](https://honglab.co.kr) 내용을 기반으로 구현하였습니다.


<br>

- **멀티스레드 렌더링 (Multi-Threaded Rendering)** 과  **Frames-in-Flight (FrameResource)** 는  
  [Microsoft DirectX 12 공식 샘플 (D3D12Multithreading)](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12Multithreading) 코드를 활용하였습니다.


<br>

---

<br>

## 멀티스레드 렌더링을 다른 방식으로 구현해보기
- 멀티스레드 렌더링 부분은 [Microsoft 공식코드](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12Multithreading)의 경우 WindowsAPI 로 되어있었는데,
- C++20 문법의 `std::barrier` 를 활용하여 원리는 동일하지만 구현방식은 바꾸어 구현해보며 멀티스레드 렌더링과  `std::barrier` 개념을 이해하고자 하였습니다.<br><br>
  (Renderer 클래스의 RenderMultithreaded 함수와 WorkerThread 함수) 

<br>

- [멀티스레드 렌더링 성능측정](https://wandering-rumba-865.notion.site/247aba645d32809c9af8cbe2723fd9e9)

<br><br>

![Diagram](MultiThread_Image.png)

<br><br>
  

