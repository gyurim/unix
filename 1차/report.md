## Writing Simple Shell

### 1차 과제
#### 요구사항 정의
  1. cd
  2. exit
  3. background

#### function
  - fatal(): 심각한 오류를 잡기 위함
  - makelist(): 명령 line을 vector 형태로 변환하는 함수
  - execute_cmdline(): makelist함수를 통해 명령어를 vector 형태로 바꾸고, cmdvector[0]에 들어있들어있는 것이 “exit”이라면 exit(1) 시스템 콜을 통해 shell을 종료시키고 “cd”라면 cmd_cd 함수를 부른다. 만약 cmdvector[count-1]에 들어있는 값(즉, 맨 뒤에 있는 값)이 “&”라면 fork를 해서 부모는 sleep(1000) 시키고 다시 prompt를 출력해 다른 명령어 입력을 받는다.
앞서 언급한 명령어들이 아니라면 fork를 해줘서 부모는 입력 받은 명령어를 그대로 수행하게 하게한다.
  - cmd_cd 함수를 통해 shell에서도 cd 명령어를 수행할 수 있게 한다. 만약 cd만 입력 받았다면 HOME으로 chdir해주고 만약 cd dirA처럼 2개의 인자를 받았다면 dirA로 chdir하게 한다.

#### 고찰
  - 좀비 프로세스가 생기는 이유
    + 자신의 작업을 완료한 자식 process가 죽으려면 부모에게 반환되어야 한다. 그러나 부모 Process가 먼저 죽어버린 상황에서는 자식 Process가 정상적으로 종료된 것이 아니기 때문에 좀비 Process가 된다.
  - 이 테스트의 문제점이 무엇인지 지적하고 문제점을 해결하기 위한 방법
    + 문제점은 좀비 Process가 생성된다는 것이다. 따라서 부모 Process가 자식 Process를 생성하고 죽을 때, 자식 Process의 부모 Process를 Init으로 연결해 자식 Process가 자신의 작업을 완료한 뒤 죽을 때 정상적으로 죽을 수 있게 해준다.  
