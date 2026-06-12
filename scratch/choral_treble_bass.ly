\version "2.24.0"

\header {
  title = "CHORAL. CORO I. II."
  subtitle = "Treble/Bass Clef Edition"
  composer = "J. S. Bach (layout adaptation)"
  tagline = ""
}

global = {
  \key bes \major
  \time 4/4
}

sopranoMusic = \relative c'' {
  \global
  r4 bes4 c4 d4 |
  d4 c4 bes4 a4 |
  g2 f4 g4 |
  a4 bes4 c2 |
  bes1 |
}

altoMusic = \relative c' {
  \global
  r4 f4 g4 a4 |
  bes4 a4 g4 f4 |
  f2 d4 e4 |
  f4 g4 a2 |
  g1 |
}

tenorMusic = \relative c' {
  \global
  r4 d4 es4 f4 |
  g4 f4 es4 d4 |
  d2 bes4 c4 |
  d4 es4 f2 |
  f1 |
}

bassMusic = \relative c {
  \global
  r4 bes4 c4 d4 |
  es4 d4 c4 bes4 |
  bes2 g4 a4 |
  bes4 c4 d2 |
  g,1 |
}

continuoMusic = \relative c {
  \global
  r4 bes4 c4 d4 |
  es4 d4 c4 bes4 |
  bes2 g4 a4 |
  bes4 c4 d2 |
  g,1 |
}

verseText = \lyricmode {
  Ich will hier bei dir ste -- hen;
  ver -- ach -- te mich doch nicht.
}

\score {
  <<
    \new ChoirStaff <<
      \new Staff = "soprano" <<
        \clef treble
        \new Voice = "sopVoice" { \sopranoMusic }
      >>
      \new Lyrics \lyricsto "sopVoice" { \verseText }

      \new Staff = "alto" <<
        \clef treble
        \new Voice { \altoMusic }
      >>

      \new Staff = "tenor" <<
        \clef treble
        \new Voice { \tenorMusic }
      >>

      \new Staff = "bass" <<
        \clef bass
        \new Voice { \bassMusic }
      >>

      \new Staff = "continuo" <<
        \clef bass
        \new Voice { \continuoMusic }
      >>
    >>
  >>
  \layout { }
}
