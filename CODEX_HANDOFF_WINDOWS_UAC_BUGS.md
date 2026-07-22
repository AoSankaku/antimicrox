# Windows の UAC 境界をまたぐ二重起動とマウス速度倍化の調査

## この文書の目的

この文書は、次の2症状を次の Codex セッションまたは開発者へ引き継ぐための調査記録である。

1. AntiMicroX のウィンドウが前面にある場合など、前面ウィンドウの条件によってマウスカーソル移動が約2倍になる。
2. Windows で通常権限版と管理者権限版の AntiMicroX を同時に起動できる。

調査対象は `master` のコミット `e0e11628`（2026-06-29）である。コード監査と一次資料の照合は行ったが、Windows 実機での再現、ビルド、修正実装はまだ行っていない。

## 結論

2症状は、別々の不具合ではなく、同じ原因から連鎖している可能性が高い。

- **確認済みの原因:** 単一起動判定が `QLocalSocket` から固定名の `QLocalServer` へ接続できるかだけで決まる。しかしサーバーに `QLocalServer::UserAccessOption` が設定されていないため、管理者権限（高整合性）で作られたWindowsローカルサーバーへ、同じユーザーの通常権限（中整合性）プロセスが接続できない。接続失敗は「既存インスタンスなし」と同じ扱いになり、2個目も完全起動する。
- **速度倍化の有力原因:** 2個のプロセスがそれぞれ同じSDLコントローラーを開き、同じプロファイルから同じ相対マウス移動を `SendInput` へ渡す。通常権限の前面ウィンドウには通常版と管理者版の両方から入力できるため、変位が合計されて約2倍になる。
- **前面条件の説明:** Windows の UIPI により、`SendInput` は呼び出し元と同じか、それより低い整合性レベルのアプリにだけ入力できる。したがって管理者権限のウィンドウが前面の場合は通常版の入力が拒否され、管理者版の1本だけになり得る。通常権限の AntiMicroX ウィンドウまたは通常アプリが前面の場合は2本とも通るため、速度差が前面ウィンドウに依存して見える。

速度倍化については、コード上の経路とWindowsの仕様が症状に強く一致するものの、実機で2プロセスの各 `SendInput` 呼び出しを計測していないため「有力原因」とする。もしタスクマネージャー上で AntiMicroX が1プロセスしか存在しない状態でも再現するなら、別原因として追加調査が必要である。

## コード上の根拠

### 1. 単一起動の判定はローカルソケット接続の成否だけである

- `src/common.h:120` の `PadderCommon::localSocketKey` は常に `"antimicroxSignalListener"` で、権限レベルを区別しない。
- `src/main.cpp:289-320` は、そのキーへ `QLocalSocket::connectToServer()` し、状態が `ConnectedState` の場合だけ既存インスタンスありと判断する。
- `src/main.cpp:296-303` では接続失敗をログに出すだけである。
- 接続できなかった場合、`src/main.cpp:379-380` で新しい `LocalAntiMicroServer` を作り、そのまま通常の起動処理へ進む。

つまり「サーバーは存在するがアクセス権のため接続できない」と「サーバーが存在しない」が区別されていない。

### 2. サーバーに権限境界をまたぐソケットオプションがない

- `src/localantimicroserver.cpp:27-30` は引数なしの `QLocalServer` を作る。
- `src/localantimicroserver.cpp:33-58` は `setSocketOptions()` を一度も呼ばずに `listen()` する。
- `src/localantimicroserver.cpp:47-54` で `listen()` が失敗してもエラー表示だけで、アプリ本体の起動を止めない。
- `src/main.cpp:379-380` も `startLocalServer()` の成功を確認できない（戻り値が `void`）。

Qt の公式ドキュメントは、Windows では `QLocalServer::UserAccessOption` を設定すれば、同じユーザーの非昇格プロセスが昇格プロセスのサーバーへ接続できると明記している。また、デフォルトはソケットオプションなしである。

さらにQtの `QLocalServer::listen()` の説明では、Windowsでは同じ名前のパイプを2つのローカルサーバーが同時にlistenできるとされている。このため、`listen()` 成否だけでも完全な排他にはならず、現在の「接続を試してからlisten」の流れには起動競合も残る。

### 3. 2個目も独立した入力処理を完全に開始する

- `src/main.cpp:558-601` は `InputDaemon`、入力スレッド、`MainWindow` を作り、ワーカーを開始する。
- `src/inputdaemon.cpp:50-61` は `SDLEventReader` を作り、SDLイベントを `InputDaemon::run()` へ接続する。
- `src/inputdaemon.cpp:153-186` は列挙したゲームコントローラーを `SDL_GameControllerOpen()` で開く。
- `src/inputdaemon.cpp:101-129` はSDLイベントを継続処理する。

プロセス間でコントローラーの所有権や出力生成を調停するコードは見当たらない。したがって、SDLが同じデバイスを複数プロセスから開くことを許す環境では、両プロセスが同じ物理入力を処理する。

### 4. 各プロセスが同じ相対マウスイベントを送る

- `src/joybuttontypes/joybutton.cpp:3624-3705` の `JoyButton::moveMouseCursor()` が変位を計算する。
- 同ファイル `:3701-3702` は変位が0でなければ `sendevent(adjustedX, adjustedY)` を呼ぶ。
- `src/event.cpp:193-194` はこれをイベントハンドラーの `sendMouseEvent()` へ渡す。
- `src/eventhandlers/winsendinputeventhandler.cpp:99-107` は `MOUSEEVENTF_MOVE` を指定した相対移動を `SendInput()` で送る。

2プロセスが同じプロファイルと入力値を処理すれば、概ね同じ相対変位が2回Windows入力ストリームへ追加される。これは「速度が正確に約2倍」という症状に一致する。

なお、`src/eventhandlers/winsendinputeventhandler.cpp:107` は `SendInput()` の戻り値を確認していない。そのためUIPIで片方が拒否されてもログから判別できない。

### 5. マウス移動経路に「AntiMicroX が前面なら2倍」の分岐はない

プロジェクト内の前面ウィンドウ参照は主に次の用途である。

- `src/autoprofilewatcher.cpp:585`: 前面アプリに応じた自動プロファイル切り替え。
- `src/gui/capturedwindowinfodialog.cpp:95`: 前面ウィンドウ情報の取得。
- `src/winextras.cpp:248-267` と `:458` 以降: `GetForegroundWindow()` を使う補助関数。

これらから `JoyButton::moveMouseCursor()` や `WinSendInputEventHandler::sendMouseEvent()` への速度係数の経路は見当たらない。よって、前面状態そのものを見てAntiMicroX内部が速度を2倍にしている、という説明はコードからは支持されない。

### 6. 権限と前面ウィンドウによる予測

通常版と管理者版が同時起動している場合、Microsoftが説明するUIPIの規則から次を予測する。

| 前面の入力先 | 通常版（中整合性）からの `SendInput` | 管理者版（高整合性）からの `SendInput` | 見込まれる合計 |
| --- | --- | --- | --- |
| 通常権限のAntiMicroX・通常アプリ・Explorer | 通る | 通る | 約2倍 |
| 管理者権限のAntiMicroX・管理者アプリ | UIPIで拒否される | 通る | 約1倍 |

この表は実機確認が必要である。特に、ユーザーが言う「AntiMicroX が前面」のウィンドウが通常版か管理者版かを、タスクマネージャーまたはProcess ExplorerでPIDと整合性レベルまで対応付ける。

## 修正方針

### 最小修正

`LocalAntiMicroServer::startLocalServer()` で `listen()` より前に、少なくともWindows向けに次を設定する。

```cpp
localServer->setSocketOptions(QLocalServer::UserAccessOption);
```

Qtの仕様上、これが「同じユーザーの管理者版が先に起動し、その後に通常版を起動する」ケースを直接修正する。アクセスを同一ユーザーに限定するため、`WorldAccessOption` より適切である。

合わせて以下を行う。

1. `startLocalServer()` を `bool` などに変更し、`listen()` 失敗時に本体の入力ワーカーを開始しない。
2. クライアント接続失敗時に `socket.error()` と `socket.errorString()` を使い、`ServerNotFoundError` と `SocketAccessError` を少なくともログ上で区別する。
3. `SendInput()` の戻り値が要求件数と異なる場合を診断ログへ出す。ただしMicrosoftの仕様上、UIPIが原因でも `GetLastError()` だけでは特定できない。

### 堅牢化

`connectToServer()` の失敗後に `listen()` する方式は原子的でなく、同時起動の競合を防げない。Qtの公式文書もWindowsで同名パイプの複数listenを許すと説明している。将来は、同一ユーザー単位の原子的なロックをIPCサーバーとは別に持たせることを検討する。

候補は `QLockFile` またはWindowsの名前付きmutexだが、次の条件を満たす必要がある。

- 通常権限と管理者権限の同一ユーザーから同じロックとして見える。
- 高整合性プロセスが先に作っても中整合性プロセスが検査できるよう、ACL/MICを正しく設定する。
- クラッシュ後のstale lockを回復できる。
- `restartAsElevated()` の引き継ぎ競合を扱う。

`src/gui/mainwindow.cpp:1397-1420` の `restartAsElevated()` は、昇格版を `ShellExecuteEx(..., "runas")` で開始してから現在の通常版を終了する。この間は意図的に2プロセスが重なるので、強い排他を追加する場合は、昇格版が旧プロセス終了を一定時間待ってからロックを取得する `--replace` 相当の設計が必要である。

## 再現・検証手順

修正前後で、同じプロファイル、同じコントローラー、同じスティック位置を使い、以下を確認する。

1. 通常版だけを起動する。一定時間のカーソル変位を基準値として記録する。
2. 管理者版だけを起動し、同じ変位を記録する。
3. 管理者版を先に起動し、通常版を後から起動する。
4. 通常版を先に起動し、管理者版を後から起動する。
5. 同じ権限の2プロセスをほぼ同時に起動する。
6. 2プロセスが存在する修正前環境で、通常版ウィンドウと管理者版ウィンドウを交互に前面にし、上表の約2倍／約1倍になるか確認する。

期待結果は次のとおり。

- 修正後は、すべての通常起動順で入力処理を行う長寿命プロセスが1個だけになる。
- 2個目は既存インスタンスへ接続し、現在のコマンド転送処理を行って終了する。
- 前面ウィンドウの権限によってカーソル変位が約2倍／約1倍に変化しない。
- `--show`、プロファイル指定、unloadなど既存のローカルソケット経由処理を壊さない。
- GUIの「管理者として再起動」も、昇格後の1プロセスだけが残る。

可能なら一時的にPID、整合性レベル、計算した `(xDis, yDis)`、`SendInput()` の戻り値を同じログへ出す。2つのPIDから同時刻に同じ変位が出れば、速度倍化の因果関係を実機で確定できる。

## 追加で確認すべき点

- 2倍症状が、AntiMicroX が1プロセスだけの状態でも再現するか。
- 症状発生時に前面だったのが通常版ウィンドウか管理者版ウィンドウか。
- SDL2/SDL3および使用ビルドで、同じ物理コントローラーを2プロセスが同時に開けること。
- `UserAccessOption` をQt 5.10と現在のQt 6ビルドの両方で設定した場合の挙動。
- `QLocalServer::removeServer()` をWindowsで毎回呼ぶ必要があるか。少なくとも排他保証としては扱わないこと。

## 一次資料

- Qt `QLocalServer`: <https://doc.qt.io/qt-6/qlocalserver.html>
  - Windowsでは `UserAccessOption` により同じユーザーの非昇格プロセスから昇格サーバーへ接続可能。
  - オプションは `listen()` より前に設定する。
  - Windowsでは同じパイプ名で2サーバーがlistenできる。
- Microsoft `SendInput`: <https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput>
  - UIPIの対象で、同じか低い整合性レベルのアプリにだけ入力を注入できる。
- Microsoft `Mandatory Integrity Control`: <https://learn.microsoft.com/en-us/windows/win32/secauthz/mandatory-integrity-control>
  - 通常プロセスは中、昇格プロセスは高整合性で、既定は lower integrity からの write-up を禁止する。
- Microsoft `Named Pipe Security and Access Rights`: <https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-security-and-access-rights>
  - 名前付きパイプのクライアント接続時にアクセスチェックが行われる。

