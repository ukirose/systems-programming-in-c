# systems-programming-in-c

C言語によるシステムプログラミング学習

---

## 目的

ふだん使っているライブラリやフレームワークが内側で何をしているのかを知るために、同じものを自分で書いています。

書いたのはシステムコールを呼ぶところまでで、その先のカーネルには踏み込んでいません。

---

## 構成

題材ごとに参考にした本が違います。

| | 中身 | 参考にした本 | 詳細 |
| :--- | :--- | :--- | :--- |
| `httpd/` | 静的ファイルの配信と CGI。接続ごとに `fork` する | ふつうのLinuxプログラミング 第2版 (青木峰郎) | [docs/httpd.md](docs/httpd.md) |
| `mylibc/` | 標準ライブラリの自作サブセット | プログラミング言語C 第2版 (K&R) | [docs/mylibc.md](docs/mylibc.md) |

```
httpd/     サーバー本体。CGI のソース、配信するファイル、テストもこの下
mylibc/    標準ライブラリの自作サブセット。httpd もこれを使う
docs/      題材ごとの構成と図
```

---

## ビルドとテスト

```sh
$ make
$ make test
build/mylibc/tests/test_my_string
13584件を比較、不一致 0件
build/mylibc/tests/test_my_malloc
12件を確認、不合格 0件
build/mylibc/tests/test_my_snprintf
266件を比較、不一致 0件
httpd/tests/http.py

合格 30 / 不合格 0
```

### 対象環境

Ubuntu 24.04.4 / aarch64 / gcc 13.3.0 で動作確認しています。
外部ライブラリは使っていません。
ビルドに必要なのは gcc と make だけです。
テストが `/proc` を読むため、走らせるには Linux が要ります。
