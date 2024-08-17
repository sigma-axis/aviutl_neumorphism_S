# AviUtl ニューモーフィズム 拡張編集フィルタプラグイン

影と光を描画してオブジェクトが出っ張っているような表現のできるフィルタ効果「ニューモーフィズムσ」を追加します．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl_neumorphism_S/releases)

![使用例](https://github.com/user-attachments/assets/0875c3bb-676d-4c35-971f-7a174614f4de)


[スクリプトでの実装](https://github.com/sigma-axis/sigma_aviutl_scripts#%E3%83%8B%E3%83%A5%E3%83%BC%E3%83%A2%E3%83%BC%E3%83%95%E3%82%A3%E3%82%BA%E3%83%A0)とほぼ同じですが，スクリプト版より高速です．

## 動作要件

- AviUtl 1.10 + 拡張編集 0.92

  http://spring-fragrance.mints.ne.jp/aviutl
  - 拡張編集 0.93rc1 等の他バージョンでは動作しません．

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

- patch.aul の `r43 謎さうなフォーク版58` (`r43_ss_58`) 以降

  https://github.com/nazonoSAUNA/patch.aul/releases/latest


## 導入方法

`aviutl.exe` と同階層にある `plugins` フォルダ内に `neumorphism_S.eef` ファイルをコピーしてください．


## 使い方

「フィルタ効果の追加」メニューから「ニューモーフィズムσ」を選んでオブジェクトに適用してください．

なるべくなら背景は無地，適用するオブジェクトは背景と同系統の色のほうが，オブジェクトが「出っ張っている」ような印象が出やすくなります．

![使用例](https://github.com/user-attachments/assets/0875c3bb-676d-4c35-971f-7a174614f4de)

### 各種パラメタ

![ニューモーフィズムσのGUI](https://github.com/user-attachments/assets/b2052eb1-76a7-4a43-b027-1e4951225073)

- 幅

  光と影の大まかな大きさをピクセル単位で指定します．正の値だとオブジェクトの背景に光と影を描画します．負の値だと内側に描画します．`0` だと何もしません．

  最小値は `-100`, 最大値は `100`, 初期値は `30`.

- ぼかし比

  光と影に適用するぼかし量を，`幅` との比で % 単位で指定します．

  最小値は `0.0`, 最大値は `500.0`, 初期値は `50.0`.

- 強さ

  光と影，双方の濃さを % 単位で指定します．実際に描画される光と影はこれに加えて `バランス` を加味したものになります．

  最小値は `0.0`, 最大値は `500.0`, 初期値は `50.0`.

- バランス

  光と影の強さのバランスを調整します．

  ![バランスの例](https://github.com/user-attachments/assets/79a35798-f121-49b2-9bfc-db226fc88a79)

  - 正の値だと影が薄く描画され，明るい背景に対して光と影の目立ち方を同程度にバランス調整できます．
  - 負の値だと光が薄く描画され，暗い背景に対して光と影の目立ち方を同程度にバランス調整できます．

  最小値は `-100.0`, 最大値は `100.0`, 初期値は `0.0`.

- 光角度

  光の配置角度を度数単位で指定します．真上が `0.0`, 時計回りに正です．

  最小値は `-720.0`, 最大値は `720.0`, 初期値は `-45.0`.

- 光色の設定

  光として描画する色を指定します．初期値は白 (`#ffffff`).

- 影色の設定

  影として描画する色を指定します．初期値は黒 (`#000000`).


## TIPS

1.  `バランス` が `+100.0` や `-100.0` に近付くと全体的に薄くなる傾向があるので，この場合 `強さ` を大きめに再調整すると全体のバランスがよくなります．

## 改版履歴

- **v1.01** (2024-08-17)

  - α値の範囲補正の計算が間違っていたのを修正．

- - **v1.00** (2024-08-17)

  - 初版．


## ライセンス

このプログラムの利用・改変・再頒布等に関しては MIT ライセンスに従うものとします．

---

The MIT License (MIT)

Copyright (C) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://mit-license.org/


#  Credits

##  aviutl_exedit_sdk

https://github.com/ePi5131/aviutl_exedit_sdk （利用したブランチは[こちら](https://github.com/sigma-axis/aviutl_exedit_sdk/tree/self-use)です．）

---

1条項BSD

Copyright (c) 2022
ePi All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
THIS SOFTWARE IS PROVIDED BY ePi “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ePi BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#  連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://x.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social

