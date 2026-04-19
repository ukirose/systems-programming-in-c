#!/usr/bin/env python3

"""自作HTTPサーバーの動作確認テストを実行するスクリプト

ビルド済みのサーバーを起動し、実際にリクエストを送信して、正しいレスポンスが返ってくるかを自動で検証します。
不正なリクエストを送りサーバーが落ちずに正しくエラーを返すかもテストします。

curl ではなく socket を直に使うのは、壊れたリクエストをそのまま送りたいのと、
接続が RST で終わったときに受信済みの応答を取りこぼさないためです。
"""

import hashlib
import os
import socket
import subprocess
import sys
import time
from contextlib import contextmanager
from pathlib import Path

# Python自身が内部で使うための絶対パス
ROOT = Path(__file__).resolve().parent.parent
DOCROOT = ROOT / "docroot"

# テスト環境の基本設定
PORT = int(os.environ.get("PORT", "8099"))
TIMEOUT_SECS = 3.0

# httpdへ渡す相対パスとコマンド
HTTPD_BIN = "./httpd"
DOCROOT_ARG = "docroot"
SERVER_ARGS = [HTTPD_BIN, str(PORT), DOCROOT_ARG]

passed = 0
failed = 0


# ==============================================================================
# エントリポイント
# ==============================================================================
def main():
    # 上のパスはリポジトリルートからの相対なので、そこへ移ってから起動する
    os.chdir(ROOT)
    server = subprocess.Popen(SERVER_ARGS, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        if not wait_until_ready():
            print(f"httpd がポート {PORT} で起動しなかった")
            return 1

        check_static_files()
        check_rejects()
    finally:
        # テストが成功しても失敗してもクラッシュしても、必ず最後にhttpdを終了
        # docroot に置いた細工は temp_path が各テストの中で消している
        server.kill()
        server.wait()

    print(f"\n合格 {passed} / 不合格 {failed}")
    return 1 if failed else 0

# ===============================================================================
# テストスイート
# ===============================================================================

# 静的ファイルが正しく取得できるかテスト
def check_static_files():
    res_index = request("/index.html")
    check("GET は 200", 200, res_index.status)
    check("拡張子から Content-Type", "text/html", res_index.header("Content-Type"))

    with temp_path("thing.unknown") as path:
        path.write_bytes(b"hi")
        res_unknown = request("/thing.unknown")
        check("未知の拡張子は octet-stream", "application/octet-stream", res_unknown.header("Content-Type"))

    # 途中で切れても前半は一致するので、全体のハッシュと長さの両方を見る
    data = os.urandom(204800)
    with temp_path("big.bin") as path:
        path.write_bytes(data)
        got = request("/big.bin")

        expected_hash = hashlib.md5(data).hexdigest()
        actual_hash = hashlib.md5(got.body).hexdigest()
        check("200KB が壊れずに届く", expected_hash, actual_hash)

        expected_len = str(len(data))
        actual_len = got.header("Content-Length")
        check("Content-Length が実長と一致", expected_len, actual_len)


# 不正なアクセスや許可されていないファイルの取得を正しく拒否(404)するかテスト
def check_rejects():
    check("存在しないパスは 404", 404, request("/nope").status)
    check("'..' は 404", 404, request("/../Makefile").status)


    check("ディレクトリは 404", 404, request("/").status)
    # lstat ではなく open で弾いている。書き込む側が居ないので O_NONBLOCK が無ければここで止まる
    with temp_path("named_pipe") as path:
        os.mkfifo(path)
        check("名前付きパイプは待たずに 404", 404, request("/named_pipe").status)

    with temp_path("outside") as path:
        path.symlink_to("/etc/passwd")
        check("docroot 外へのリンクは 404", 404, request("/outside").status)

    # %00 を復元する実装なら index.html に化ける。復元しないので、ただの名前として扱われる
    check("パスの %00 は復元しない", 404, request("/index.html%00.txt").status)


# 通信・判定ヘルパー
# ==============================================================================

# リクエストを組み立てて送る
def request(target, method="GET", headers=()):
    lines = [f"{method} {target} HTTP/1.0".encode()]
    lines += [h.encode() for h in headers]

    return send(b"\r\n".join(lines) + b"\r\n\r\n")


# 生のバイト列を送り、接続が閉じるまで読む
def send(raw):
    received = b""
    try:
        with socket.create_connection(("127.0.0.1", PORT), timeout=TIMEOUT_SECS) as sock:
            sock.sendall(raw)
            while True:
                chunk = sock.recv(65536)
                if not chunk:
                    break
                received += chunk

    except (ConnectionResetError, BrokenPipeError):
        # 上限超過のリクエストでは、サーバーが未読のまま閉じるので RST になる
        # そこまでに受け取った分は有効なので、握り潰して返す
        pass
    except socket.timeout:
        return Response(b"")

    return Response(received)


# 応答のバイト列を、ステータス・ヘッダ・本文に分ける
class Response:
    def __init__(self, raw):
        # 空行 (\r\n\r\n) がヘッダと本文の境目
        head, _, self.body = raw.partition(b"\r\n\r\n")
        lines = head.split(b"\r\n")

        # 壊れた応答や空の応答でも例外にせず、status を 0 にして比較へ回す
        status_line_words = lines[0].split()
        if len(status_line_words) >= 2 and status_line_words[1].isdigit():
            self.status = int(status_line_words[1])
        else:
            self.status = 0

        # ヘッダ名は大小を無視して引きたいので、小文字に寄せて持つ
        self.headers = {}
        for line in lines[1:]:
            name, _colon, value = line.partition(b":")
            if _colon:
                header_name = name.decode().lower()
                header_value = value.strip().decode()
                self.headers[header_name] = header_value

    # 無いヘッダは "" を返す。比較で None を気にせずに済む
    def header(self, name):
        return self.headers.get(name.lower(), "")


# テストの合否を判定し、スコアを記録するジャッジ関数
def check(name, want, got):
    global passed, failed

    if got == want:
        passed += 1
        return

    failed += 1
    print(f"NG  {name}\n"
          f"期待: {want!r}\n"
          f"実際: {got!r}")


# 接続できるようになるまで待つ。固定の sleep より速く、かつ確実
def wait_until_ready():
    deadline = time.monotonic() + TIMEOUT_SECS
    while time.monotonic() < deadline:
        try:
            socket.create_connection(("127.0.0.1", PORT), timeout=0.2).close()
            return True
        except OSError:
            time.sleep(0.05)

    return False


# ==============================================================================
# 低レイヤユーティリティ（ファイル・プロセス操作）
# ==============================================================================

# docroot 直下のパスを貸し出し、with を抜けるとき必ず消す
# 中身の作り方は呼ぶ側に任せる。ファイル・ディレクトリ・パイプ・リンクを 1つで扱うため
@contextmanager
def temp_path(name):
    path = DOCROOT / name
    try:
        yield path
    finally:
        # ディレクトリだけ rmdir、それ以外(ファイル・パイプ・リンク)は unlink で消える
        if path.is_dir() and not path.is_symlink():
            path.rmdir()
        else:
            path.unlink(missing_ok=True)


if __name__ == "__main__":
    sys.exit(main())
