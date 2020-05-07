# cmdReceiver

### [cmdReceiver.c](https://github.com/indra-kr/Tools/blob/master/cmdReceiver/cmdReceiver.c)

가끔 우리는 모의해킹 중 어려운 환경을 만날때가 있습니다.  
예를 들어 웹쉘만 사용 가능한 환경 (inbound 80 포트만 통신이 되는 환경)이 그런 케이스인데요.  
그런 환경에서 사용할 수 있는 방법입니다.  
임의의 명령어를 실행할 수 있고, 권한 상승 취약점의 exploit이 가능할 때  
이 프로그램에 setuid bit를 설정하도록 exploit하여, 타 권한의 명령어 실행 결과를 웹쉘로 출력할 수 있습니다.  
<details>
<summary>JP</summary>
たまにはPEN-Testの中に細かい環境に出会う事かあります。<BR>
例えば外部に出られるポートーが全部ブロークされて、Webshellしか利用できない環境。
その環境で使える方法です。<BR>
あるコマンドを実行できるし、Privilige Escalation脆弱性か存在する場合、<BR>
グラムにターゲットの権限のSetuidビートを設定するように攻撃したらその権限でコマンドを実行できます。<BR>
</details>
<details>
<summary>EN</summary>
Sometimes, we have to challenging in pen-testing.<BR>
For example, we can just use a webshell environment—The environment that able to communicate through inbound 80 port only.<BR>
On that environment, we can use this.<BR>
If we can run arbitrary command on the target server and able to attack the privilege escalation vulnerability,<BR>we can run commands by other privilege through setup the setuid bit to this program.<BR>
</details>

```
                                   |            Area of processes
+---------+     +---------------+     +-------------+     +---------
| Browser | <-> | Target Server | <-> | cmdReceiver | <-> | Commands ...
+---------+     +---------------+     +-------------+     \_______
             ^ Communication       |                   ^ running by escalated privilege 
```
