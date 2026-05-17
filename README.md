# Azahar BBP Optimization

## 日本語

これは、バンブラP（Daigasso! Band Brothers P / BBP）の最適化と検証を目的にした、個人用の Azahar fork です。

このリポジトリは、BBP 向けのパッチ、診断コード、実験ビルドを公開・整理するための場所です。汎用の Azahar 配布版を目指すものではなく、upstream への取り込みや一般用途への適合を前提にしません。

## 目的

- BBP の描画やパフォーマンス挙動に絞った Azahar fork を管理する。
- BBP 最適化パッチや実験ビルドを、個人用の公開リポジトリとして残す。
- 診断コード、レンダリング修正、framebuffer 周りの実験などをひとつの履歴にまとめる。
- 将来的に使えそうな変更も、まずは BBP 専用の形で扱えるようにする。

## スコープ

この fork は BBP 専用の作業と個人的な実験を対象にしています。変更には、エミュレータ側の診断コード、描画修正、framebuffer handling の実験、パフォーマンス最適化、一時的な調査コードが含まれる場合があります。

すべての変更を upstream に提出できる形へ整える必要はありません。必要になったものだけ、後から整理して再利用する方針です。

## ベースプロジェクト

Azahar は Citra をベースにしたオープンソースの Nintendo 3DS エミュレータです。

Base project:

https://github.com/azahar-emu/azahar

## ライセンス

このリポジトリは upstream Azahar のライセンスに従います。詳細は `license.txt` を参照してください。

---

## English

This is a personal Azahar fork dedicated to optimizing and testing Daigasso!
Band Brothers P, known in Japanese as バンブラP and also referred to as BBP.

This repository is a personal publication and experiment space for BBP-focused
patches, diagnostics, and builds. It is not intended to be a general-purpose
Azahar distribution, and changes do not need to satisfy upstream acceptance
criteria by default.

## Purpose

- Maintain a personal Azahar fork focused on BBP rendering and performance.
- Publish BBP optimization patches and experimental builds in one public place.
- Keep diagnostics, rendering fixes, framebuffer experiments, and related work
  in a stable history.
- Allow useful changes to start as BBP-specific work before any broader cleanup.

## Scope

This fork is intended for BBP-specific work and personal experimentation.
Changes may include emulator-side diagnostics, rendering fixes, framebuffer
handling experiments, performance optimizations, and temporary investigation
code useful for Band Brothers P.

Not every change is expected to be suitable for upstream submission. If a patch
becomes broadly useful, it can be cleaned up later.

## Base Project

Azahar is an open-source Nintendo 3DS emulator based on Citra.

Base project:

https://github.com/azahar-emu/azahar

## License

This repository follows the upstream Azahar license. See `license.txt`.
