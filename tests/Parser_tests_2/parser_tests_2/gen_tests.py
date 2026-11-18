#!/usr/bin/env python3
"""Generator testu pro parser IFJ26. Pouziti: python3 gen_tests.py <vystupni_slozka>"""
import os
import sys
import textwrap

OUT = sys.argv[1]
TESTS = []  # (dir, name, codes, src, note)

MAIN = "@main def main(): Unit =\n  print(0)\n"


def t(d, name, codes, src, note="", final_nl=True, raw=False):
    if not raw:
        src = textwrap.dedent(src).lstrip("\n")
    src = src.replace("<SP>", " ").replace("<TAB>", "\t")
    src = src.rstrip("\n") + ("\n" if final_nl else "")
    TESTS.append((d, name, codes, src, note))


def ok(name, src, note="", **kw):
    t("ok", name, "0", src, note, **kw)


def lex(name, src, note="", **kw):
    t("err_lex", name, "1", src, note, **kw)


def syn(name, src, note="", **kw):
    t("err_syntax", name, "2", src, note, **kw)


def sem(name, codes, src, note=""):
    t("err_sem", name, codes, src, note)


def base(name, codes, ext, src, note=""):
    t("base_only", name, codes, src,
      f"V zakladu chyba, ale s rozsirenim {ext} je to platny kod. "
      f"Oficialni testy to v zakladu nekontroluji. {note}".strip())


def unclear(name, codes, src, note=""):
    t("unclear", name, codes, src, note)


# =====================================================================
# OK: syntakticky i semanticky spravne programy (ocekavano 0)
# =====================================================================
ok("01_minimal", """
import ifj26.*
@main def main(): Unit =
  print(1)
""")

ok("02_main_annotation_own_line", """
import ifj26.*

@main
def main(): Unit =
  print(1)
""", "Slajd 22: mezi @main a def muze byt <NL*>.")

ok("03_demo_slide22_exact", """
import ifj26.*

def add(x: Int, y: Int): Int =
  return x + y

@main def main(): Unit =
  var x: Int = 3 + 11
  val y = add(x, 1,
  )
  x = (x + y)
  while x > 0 do
    if (x < 10) then
      print(x)
    else
      print(y)
    x = x - 1
  print(x); print(y)
""", "Presne program ze slajdu 22 dema (vcetne koncove carky s NL pred zavorkou).")

ok("04_demo_slide22_no_trailing_comma", """
import ifj26.*

def add(x: Int, y: Int): Int =
  return x + y

@main def main(): Unit =
  var x: Int = 3 + 11
  val y = add(x, 1)
  x = (x + y)
  while x > 0 do
    if (x < 10) then
      print(x)
    else
      print(y)
    x = x - 1
  print(x); print(y)
""")

ok("05_demo_slide6", """
import ifj26.*
def fun(x: Int): Int =
  val y = x * 3
  print(y)
  y - 10

@main def main(): Unit =
  val r = fun(4)
  print(r)
""", "Implicitni navratova hodnota = posledni vyraz.")

ok("06_demo_slide37_forward_call_inferred_type", """
import ifj26.*
def foo(x: Int): Int =
  var a = bar(x)
  a

def bar(y: Int) =
  y * 10

@main def main(): Unit =
  val r = foo(2)
  print(r)
""", "Volani funkce pred jeji definici + odvozeny navratovy typ.")

ok("07_demo_slide40_scopes", """
import ifj26.*
def fun(x: Int): Unit =
  // val x = 0 // redefinice
  // x = 1 // parametr je val
  var y = 1
  while y > 0 do
    var x = 20 // prekryti lze
    x = x - 1
    y = y - 1
  if x < 2 then
    var x = 10 // dalsi scope
    x = x + 1
  else
    ()
  print(x) // puvodni parametr

@main def main(): Unit =
  fun(1)
""")

ok("08_demo_slide52_nested_while_shadowing", """
import ifj26.*
def fun(n: Int) =
  var x = n
  while x > 5 do
    var y = 10 * x
    while y < 100 do
      var x = 5 - y
      print(x)
      y = y - 1
    x = x - 2

@main def main(): Unit =
  fun(9)
""", "Telo funkce konci cyklem -> navratovy typ Unit. Vicenasobny DEDENT najednou.")

ok("09_demo_slide36_string_compare", """
import ifj26.*
@main def main(): Unit =
  val x = "foo"
  val z = "brb"
  if x + "abc" < z then
    ()
  else
    ()
""", "Unit literal () jako prikaz, konkatenace a lexikograficke porovnani.")

ok("10_demo_slide25_block", """
import ifj26.*
def run(n: Int): Unit =
  var x = n
  while x > 0 do
    print(x)
    x = x - 1
  print("konec")

@main def main(): Unit =
  run(3)
""")

ok("11_params_multiline", """
import ifj26.*
def add(
  x: Int,
  y: Int
): Int =
  return x + y

@main def main(): Unit =
  val s = add(1, 2)
  print(s)
""", "Uvnitr zavorek je NL jen bily znak.")

ok("12_newlines_everywhere_in_parens", """
import ifj26.*
def add(
x
:
Int
,

y
  :
    Int
): Int =
  return x + y

@main def main(): Unit =
  val s = add(
    1
    ,
        2
  )
  print(s)
""", "Slajd 22: <NL*> pred/za : a , uvnitr zavorek, i prazdne radky a libovolne odsazeni.")

ok("13_return_type_omitted", """
import ifj26.*
def times10(y: Int) =
  y * 10

def greet() =
  print("ahoj")

@main def main(): Unit =
  val a = times10(3)
  print(a)
  greet()
""", "Navratovy typ lze vynechat (funkce bez return a neni rekurzivni).")

ok("14_all_types", """
import ifj26.*
def f(a: Int, b: Double, c: String): Unit =
  print(a)
  print(b)
  print(c)

def g(): Double =
  return 1.5

def h(): String =
  return "s"

def u(): Unit =
  ()

@main def main(): Unit =
  f(1, 2.5, "x")
  val d = g()
  val s = h()
  u()
""")

ok("15_val_var_typed_untyped", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  val b: Int = 2
  var c = 3.0
  var d: Double = 4.5
  val e = "e"
  var f: String = "f"
  c = d
  f = e
  val sum = a + b
  print(sum)
""")

ok("16_nested_if_dedent_to_else", """
import ifj26.*
@main def main(): Unit =
  val a = 5
  if a > 0 then
    if a > 10 then
      print(1)
    else
      print(2)
  else
    if a < -10 then
      print(3)
    else
      print(4)
  print(5)
""", "Dva DEDENTy za sebou a hned potom else.")

ok("17_deep_nesting", """
import ifj26.*
@main def main(): Unit =
  var i = 3
  while i > 0 do
    var j = 3
    while j > 0 do
      if i == j then
        if i != 2 then
          while j > 100 do
            print(j)
        else
          print(0)
      else
        print(1)
      j = j - 1
    i = i - 1
  print(i)
""", "Pet urovni; navrat o 3 urovne najednou.")

ok("18_dedent_many_at_eof_no_newline", """
import ifj26.*
@main def main(): Unit =
  var i = 1
  while i > 0 do
    if i == 1 then
      while i > 5 do
        print(i)
    else
      print(0)
    i = i - 1
""", "Soubor konci bez \\n -> na EOF se musi vygenerovat vsechny zbyvajici DEDENTy.", final_nl=False)

ok("19_trailing_blank_lines", """
import ifj26.*
@main def main(): Unit =
  print(1)



""")
TESTS[-1] = (TESTS[-1][0], TESTS[-1][1], TESTS[-1][2], TESTS[-1][3] + "\n\n\n<SP><SP>\n".replace("<SP>", " "), TESTS[-1][4])

ok("20_blank_and_whitespace_lines", """
import ifj26.*


def f(x: Int): Int =
<SP><SP><SP><SP><SP><SP>
  var y = x
<SP>
  while y > 0 do

<SP><SP><SP><SP><SP><SP><SP><SP><SP><SP>
    y = y - 1
<SP><SP>

  return y



@main def main(): Unit =

  val r = f(3)
  print(r)
""", "Prazdne radky a radky jen s mezerami (s libovolnym poctem mezer) nemeni odsazeni.")

ok("21_comments_everywhere", """
// komentar pred prologem
import ifj26.* // komentar za prologem
// komentar mezi
def f(x: Int): Int = // za hlavickou
  // komentar na zacatku bloku
  var y = x // za prikazem
// komentar uplne vlevo uvnitr bloku
          // komentar hodne vpravo uvnitr bloku
  while y > 0 do // za do
    // v cyklu
    y = y - 1
  // po cyklu, odsazeni bloku
    // po cyklu, vic odsazeny
  return y
// konec funkce

@main // za anotaci
def main(): Unit =
  val r = f(2)
  print(r)
// komentar na konci bez noveho radku""", "Komentarove radky nemeni odsazeni, ani kdyz jsou mene/vice odsazene.", final_nl=False)

ok("22_block_comments", """
import ifj26.*
/* blokovy
   komentar
   pres vic radku */
def f(x: Int): Int =
  val a = /* uvnitr vyrazu */ x + /* dalsi */ 1
  /* komentar
na zacatku radku s malym odsazenim */
  val b = a /* za vyrazem */
  /* vnoreny /* komentar */ stale komentar */
  return b

@main def main(): Unit =
  val r = f(1)
  print(r)
""", "Vnorene blokove komentare (/* /* */ */) podle zadani.")

ok("23_semicolons", """
import ifj26.*
@main def main(): Unit =
  val a = 1; val b = 2; var c = a + b
  c = c * 2; print(c)
  if a < b then
    print(a); print(b)
  else
    print(c); c = 0
  while c > 0 do
    c = c - 1; print(c)
""")

ok("24_expression_precedence", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  val b = 2
  val c = 3
  val d = 4
  val e = 5
  val r1 = a + b * c - d / e
  val r2 = (a + b) * (c - d)
  val r3 = -a
  val r4 = - -a
  val r5 = +a
  val r6 = a * -b
  val r7 = -(a + b) * 2
  val r8 = ((((a))))
  val r9 = a - - b
  val r10 = a - -b + +c
  val r11 = (a)
  val r12 = 10 - 2 - 3
  val r13 = 100 / 10 / 2
  val r14 = 2 * (3 + 4) * 5
  val r15 = -(-(-(1)))
  print(r1)
""", "Unarni +/- s nejvyssi prioritou a pravou asociativitou.")

ok("25_relational_operators", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  val b = 2
  if a < b then
    print(1)
  else
    print(0)
  if a <= b then
    print(1)
  else
    print(0)
  if a > b then
    print(1)
  else
    print(0)
  if a >= b then
    print(1)
  else
    print(0)
  if a == b then
    print(1)
  else
    print(0)
  if a != b then
    print(1)
  else
    print(0)
  if a + 1 <= b * 2 - 3 then
    print(1)
  else
    print(0)
  if (a) < (b) then
    print(1)
  else
    print(0)
  if (a + b) * 2 != -a then
    print(1)
  else
    print(0)
  while (a > b) do
    print(a)
""")

ok("26_strings", r"""
import ifj26.*
@main def main(): Unit =
  val s1 = "obycejny retezec"
  val s2 = ""
  val s3 = "escape: \" \\ \n \t \r konec"
  val s4 = "  mezery  "
  val s5 = "// neni komentar"
  val s6 = "/* taky neni komentar */"
  val s7 = "print(x) + 1 ; if then else"
  val s8 = s1 + s2 + s3
  if s1 == "abc" then
    print(s1)
  else
    print(s8)
""", "Jen escape sekvence ze zadani: \\\\ \\\" \\n \\r \\t.")

ok("27_multiline_string", r'''
import ifj26.*
@main def main(): Unit =
  val s = """prvni radek
druhy radek s "uvozovkami" a \n neni escape
    treti radek odsazeny"""
  print(s)
  val t = """"""
  print(t)
''', "Viceradkovy retezec: escape sekvence se nevyhodnocuji, odsazeni uvnitr nehraje roli.")

ok("28_numbers", """
import ifj26.*
@main def main(): Unit =
  val a = 0
  val b = 1_000_000
  val c = 3.14
  val d = 0.5
  val e = 1e10
  val f = 2.5E-3
  val g = 1.0e+2
  val h = 12_345
  val i = 1_0
  print(a)
""", "Podtrzitka jako oddelovace cislic, exponent s/bez znamenka.")

ok("29_call_forms", """
import ifj26.*
def f(a: Int, b: String, c: Double, d: Int): Int =
  return a

def g(): Int =
  return 7

@main def main(): Unit =
  val x = 1
  f(1, "a", 2.5, x)
  val r = f(x, "b", 0.0, x)
  var q: Int = g()
  q = f(q, "c", 1.0, 2)
  q = g()
  print(q)
""", "Volani jako prikaz, jako inicializace val/var (s typem i bez) a jako prava strana prirazeni.")

ok("30_call_args_multiline", """
import ifj26.*
def f(a: Int, b: Int, c: Int): Int =
  return a + b + c

@main def main(): Unit =
  val r = f(
    1,
    2,
    3
  )
  val s = f(1,
            2, 3)
  print(r)
""")

ok("31_trailing_comma_args_with_newline", """
import ifj26.*
def f(a: Int, b: Int): Int =
  return a + b

@main def main(): Unit =
  val r = f(
    1,
    2,
  )
  print(r)
""", "Slajd 22 a 56: koncova carka je povolena, ale jen kdyz pred ')' je NL.")

ok("32_recursion_factorial", """
import ifj26.*
def fact(n: Int): Int =
  if n <= 1 then
    return 1
  else
    val m = n - 1
    val sub = fact(m)
    return n * sub

@main def main(): Unit =
  val n = readInt()
  val r = fact(5)
  print(r)
""", "Rekurzivni funkce s explicitnim typem; obe vetve if konci return.")

ok("33_factorial_iterative", """
import ifj26.*
def fact(n: Int): Int =
  var res = 1
  var i = n
  while i > 1 do
    res = res * i
    i = i - 1
  return res

@main def main(): Unit =
  val r = fact(6)
  print(r)
""")

ok("34_mutual_recursion", """
import ifj26.*
def isEven(n: Int): Int =
  if n == 0 then
    return 1
  else
    val m = n - 1
    val r = isOdd(m)
    return r

def isOdd(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val r = isEven(m)
    return r

@main def main(): Unit =
  val r = isEven(10)
  print(r)
""")

ok("35_main_first", """
import ifj26.*
@main def main(): Unit =
  val r = helper(1)
  print(r)

def helper(x: Int): Int =
  return x + 1
""", "@main nemusi byt posledni funkce.")

ok("36_main_not_named_main", """
import ifj26.*
@main def start(): Unit =
  print("start")
""", "Vstupni funkce je ta s @main, jmeno muze byt libovolne.")

ok("37_identifiers", """
import ifj26.*
def _f(_a: Int, a_1: Int): Int =
  return _a + a_1

@main def main(): Unit =
  val x = 1
  val X = 2
  val __a__ = 3
  val camelCase = 4
  val ifx = 5
  val define = 6
  val values = 7
  val whilex = 8
  val returnValue = 9
  val Integer = 10
  val doo = 11
  val thenx = 12
  val elsewhere = 13
  val Unitx = 14
  val ifj26 = 15
  val r = _f(x, X)
  print(r)
""", "Case-sensitive, podtrzitka, klicova slova jako prefix identifikatoru.")

ok("38_expression_newlines_in_parens", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  val y = 2
  x = (x +
       y)
  x = (
    x
    *
    y
  )
  if (
    x
    <
    10
  ) then
    print(x)
  else
    print(y)
  while (x
    > 0) do
    x = x - 1
""", "Slajd 22/33: uvnitr zavorek muze byt NL kdekoliv, i za operatorem.")

ok("39_return_forms", """
import ifj26.*
def a(x: Int): Int =
  return x

def b(x: Int): Int =
  return (x + 1) * 2

def c(): String =
  return "s"

def d(x: Int): Int =
  var i = x
  while i > 0 do
    if i == 5 then
      return i
    else
      i = i - 1
  return 0

def e(x: Int): Int =
  return -x

@main def main(): Unit =
  val r = d(10)
  print(r)
""")

ok("40_else_contains_if_chain", """
import ifj26.*
def sign(x: Int): Int =
  if x < 0 then
    return -1
  else
    if x == 0 then
      return 0
    else
      return 1

@main def main(): Unit =
  val m = -5
  val s = sign(m)
  print(s)
""", "Bez BOOLTHEN neexistuje 'else if' na jednom radku; vnoreny if v bloku else.")

ok("41_many_functions", """
import ifj26.*
def f1(): Int =
  return 1
def f2(a: Int): Int =
  return a
def f3(a: Int, b: Int): Int =
  return a + b
def f4(a: String): String =
  return a
def f5(a: Double): Double =
  return a
def f6(): Unit =
  print(6)
def f7(a: Int, b: Int, c: Int, d: Int, e: Int, f: Int): Int =
  return a + b + c + d + e + f
def f8() =
  8
def f9(x: Int) =
  x
@main def main(): Unit =
  val a = f1()
  val b = f2(a)
  val c = f3(a, b)
  val d = f4("d")
  val e = f5(1.0)
  f6()
  val g = f7(1, 2, 3, 4, 5, 6)
  val h = f8()
  val i = f9(9)
  print(g)
""", "Funkce bez prazdnych radku mezi sebou.")

ok("42_different_indent_widths", """
import ifj26.*
def f(x: Int): Int =
 var y = x
 while y > 0 do
         y = y - 1
         if y == 3 then
          print(y)
         else
                                print(0)
 return y

@main def main(): Unit =
    val r = f(5)
    print(r)
""", "Pocet mezer neni pevne dany: 1, 8, 9, 32 mezer.")

ok("43_unit_function_unit_literal", """
import ifj26.*
def nop(): Unit =
  ()

def nop2() =
  ()

@main def main(): Unit =
  nop()
  nop2()
  ()
""")

ok("44_implicit_int_to_double", """
import ifj26.*
def half(x: Double): Double =
  return x / 2

@main def main(): Unit =
  val d: Double = 1
  val e = d + 1
  var f: Double = 2
  f = 3
  f = f * 2 + 1
  val g = 1 + 2.5
  if d < 2 then
    print(e)
  else
    print(g)
""", "Jedina implicitni konverze Int -> Double (prirazeni i ve vyrazu).")

ok("45_semicolons_nested", """
import ifj26.*
@main def main(): Unit =
  var i = 3
  while i > 0 do
    if i == 2 then
      print(2); print(22)
    else
      print(i); i = i - 0
    i = i - 1; print(i)
  print(100)
""")

ok("46_long_program", """
import ifj26.*

// vypocet n-teho Fibonacciho cisla, prvocisla a prace s retezci

def fib(n: Int): Int =
  if n < 2 then
    return n
  else
    val a = n - 1
    val b = n - 2
    val fa = fib(a)
    val fb = fib(b)
    return fa + fb

def isPrime(n: Int): Int =
  if n < 2 then
    return 0
  else
    var d = 2
    while d * d <= n do
      val q = n / d
      if q * d == n then
        return 0
      else
        d = d + 1
    return 1

def repeat(s: String, n: Int): String =
  var res = ""
  var i = 0
  while i < n do
    res = res + s
    i = i + 1
  return res

@main def main(): Unit =
  val f = fib(10)
  print(f)
  var k = 0
  while k < 20 do
    val p = isPrime(k)
    if p == 1 then
      print(k)
    else
      ()
    k = k + 1
  val line = repeat("-", 10)
  print(line)
""")

ok("47_builtins", """
import ifj26.*
@main def main(): Unit =
  val s = readLine()
  val i = readInt()
  val d = readDouble()
  val t = toStr(i)
  val k = toInt(d)
  val n = strLength(t)
  val sub = subStr(t, 0, 1)
  val o = ord(t, 0)
  val c = chr(65)
  print(sub)
""", "Pokud tohle spadne az v semantice (kod 5), over presne signatury vestavenych funkci v zadani.")

ok("48_statement_after_block_same_level", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  if x > 0 then
    x = 2
  else
    x = 3
  x = 4
  while x > 0 do
    x = x - 1
  x = 5
  if x > 0 then
    x = 2
  else
    x = 3
""", "Za DEDENTem bloku hned dalsi prikaz bez prazdneho radku.")

ok("49_while_then_if_dedent", """
import ifj26.*
@main def main(): Unit =
  var x = 3
  if x > 0 then
    while x > 0 do
      x = x - 1
  else
    while x < 0 do
      x = x + 1
""")

ok("50_tab_inside_line", "import ifj26.*\n@main def main(): Unit =\n  val x\t=\t1\n  print(x)<TAB>// tab za prikazem\n",
   "Tabulator uprostred radku je bily znak, chyba je jen na zacatku radku.", raw=True)

ok("51_tab_on_empty_line", "import ifj26.*\n@main def main(): Unit =\n  val x = 1\n<TAB>\n  print(x)\n",
   "Zadani: chyba je tab na zacatku NEPRAZDNEHO radku. Radek jen s tabem je prazdny.", raw=True)

ok("52_parenthesized_operands_and_unary_in_conditions", """
import ifj26.*
@main def main(): Unit =
  val a = 2
  if -a < -(1) then
    print(1)
  else
    print(2)
  if ((a)) == ((2)) then
    print(3)
  else
    print(4)
""")

ok("53_double_ops", """
import ifj26.*
@main def main(): Unit =
  val a = 1.5
  val b = -a
  val c = a * b / 2.0 - 0.25
  if c >= 0.0 then
    print(c)
  else
    print(b)
""")

ok("54_shadow_in_both_branches", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    val x = "retezec"
    print(x)
  else
    val x = 2.5
    print(x)
  while x < 0 do
    val x = 3
    print(x)
  print(x)
""", "Prekryti v podbloku je povolene (i se zmenou typu).")

ok("55_string_escape_edge", r"""
import ifj26.*
@main def main(): Unit =
  val a = "\\"
  val b = "\""
  val c = "\\\""
  val d = "a\\nb"
  print(a)
""")

ok("56_comment_like_inside_string_multiline", r'''
import ifj26.*
@main def main(): Unit =
  val s = """ /* neni komentar
// taky ne
"""
  print(s)
''')

ok("57_assignment_from_var", """
import ifj26.*
@main def main(): Unit =
  var a = 1
  var b = 2
  a = b
  b = a
  a = (b)
  print(a)
""")

ok("58_return_unit_literal_function", """
import ifj26.*
def f(x: Int): Unit =
  if x > 0 then
    print(x)
  else
    ()

@main def main(): Unit =
  f(1)
""", "Funkce s Unit konci if-em: povoleno.")

ok("59_if_both_branches_unit", """
import ifj26.*
def f(x: Int) =
  if x > 0 then
    print(x)
  else
    print(0)

@main def main(): Unit =
  f(1)
""", "Funkce bez typu konci if-em, obe vetve typu Unit.")

ok("60_block_comment_between_statements", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  /* viceradkovy
     komentar mezi prikazy */
  val b = 2
  /**/
  print(b)
""")

# =====================================================================
# LEXIKALNI CHYBY (1)
# =====================================================================
lex("01_tab_at_line_start", "import ifj26.*\n@main def main(): Unit =\n\tprint(1)\n", raw=True)
lex("02_tab_after_spaces_at_line_start", "import ifj26.*\n@main def main(): Unit =\n  \tprint(1)\n", raw=True,
    note="Tab v uvodnim odsazeni (i za mezerami).")
lex("03_tab_in_nested_block", "import ifj26.*\n@main def main(): Unit =\n  var x = 1\n  while x > 0 do\n\t\tx = x - 1\n", raw=True)
for i, ch in enumerate(["#", "$", "%", "?", "^", "~", "`", "&", "|", "\\", "'"]):
    lex(f"{10+i}_unknown_char_{ord(ch)}", f"""
    import ifj26.*
    @main def main(): Unit =
      val x = 1 {ch} 2
    """, f"Neznamy znak '{ch}'.")
lex("30_unterminated_string", """
import ifj26.*
@main def main(): Unit =
  val s = "neukonceny
  print(s)
""")
lex("31_unterminated_string_eof", 'import ifj26.*\n@main def main(): Unit =\n  val s = "abc', raw=True)
lex("32_invalid_escape", r"""
import ifj26.*
@main def main(): Unit =
  val s = "a\qb"
""")
lex("33_invalid_escape_backslash_space", r"""
import ifj26.*
@main def main(): Unit =
  val s = "a\ b"
""")
lex("34_unterminated_block_comment", """
import ifj26.*
@main def main(): Unit =
  print(1)
/* neukonceny komentar
""")
lex("35_unterminated_nested_comment", """
import ifj26.*
@main def main(): Unit =
  print(1)
/* vnejsi /* vnitrni */ vnejsi neukonceny
""", "Komentare se vnoruji -> po prvnim */ je stale v komentari.")
lex("36_unterminated_multiline_string", '''
import ifj26.*
@main def main(): Unit =
  val s = """abc
  print(s)
''')
lex("37_exponent_without_digits", """
import ifj26.*
@main def main(): Unit =
  val d = 1e
""")
lex("38_exponent_sign_without_digits", """
import ifj26.*
@main def main(): Unit =
  val d = 1.5e+
""")
lex("39_number_trailing_underscore", """
import ifj26.*
@main def main(): Unit =
  val d = 1_
""")
lex("40_non_ascii_identifier", """
import ifj26.*
@main def main(): Unit =
  val čau = 1
""")
lex("41_lone_exclamation_mark", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x ! 2 then
    print(1)
  else
    print(2)
""", "Samotny '!' (ne '!=') v zakladu neni lexem.")
lex("42_tab_before_def", "import ifj26.*\n\t@main def main(): Unit =\n  print(1)\n", raw=True)
lex("43_number_fraction_underscore_end", """
import ifj26.*
@main def main(): Unit =
  val d = 1.5_
""")
lex("44_string_newline_inside_args", """
import ifj26.*
@main def main(): Unit =
  print("abc
  def")
""", "Ani uvnitr zavorek nesmi bezny retezec obsahovat novy radek.")

# =====================================================================
# SYNTAKTICKE CHYBY (2)
# =====================================================================
# --- prolog ---
syn("001_missing_prolog", """
@main def main(): Unit =
  print(1)
""")
syn("002_prolog_wrong_name", """
import ifj25.*
@main def main(): Unit =
  print(1)
""")
syn("003_prolog_missing_star", """
import ifj26.
@main def main(): Unit =
  print(1)
""")
syn("004_prolog_missing_dot", """
import ifj26*
@main def main(): Unit =
  print(1)
""")
syn("005_prolog_missing_module", """
import .*
@main def main(): Unit =
  print(1)
""")
syn("006_prolog_twice", """
import ifj26.*
import ifj26.*
@main def main(): Unit =
  print(1)
""")
syn("007_prolog_after_def", """
@main def main(): Unit =
  print(1)
import ifj26.*
""")
syn("008_prolog_and_def_same_line", """
import ifj26.* @main def main(): Unit =
  print(1)
""", "Za prologem je vyzadovan <NL+>.")
syn("009_prolog_other_module", """
import scala.*
@main def main(): Unit =
  print(1)
""")
syn("010_prolog_import_only", """
import
@main def main(): Unit =
  print(1)
""")
syn("011_prolog_split_lines", """
import
ifj26.*
@main def main(): Unit =
  print(1)
""")
syn("012_only_prolog_and_garbage", """
import ifj26.*
xyz
""")
syn("013_empty_file", "", raw=True)
syn("014_prolog_case", """
Import ifj26.*
@main def main(): Unit =
  print(1)
""", "Case-sensitive: Import neni klicove slovo.")

# --- definice funkci ---
syn("020_def_missing_name", """
import ifj26.*
def (x: Int): Int =
  return x
""" + MAIN)
syn("021_param_missing_type", """
import ifj26.*
def f(x): Int =
  return 1
""" + MAIN)
syn("022_param_missing_colon", """
import ifj26.*
def f(x Int): Int =
  return x
""" + MAIN)
syn("023_param_colon_without_type", """
import ifj26.*
def f(x:): Int =
  return 1
""" + MAIN)
syn("024_params_double_comma", """
import ifj26.*
def f(x: Int,, y: Int): Int =
  return x
""" + MAIN)
syn("025_params_leading_comma", """
import ifj26.*
def f(, x: Int): Int =
  return x
""" + MAIN)
syn("026_params_trailing_comma_same_line", """
import ifj26.*
def f(x: Int,): Int =
  return x
""" + MAIN, "Koncova carka bez NL pred ')' neni povolena.")
syn("027_params_missing_comma", """
import ifj26.*
def f(x: Int y: Int): Int =
  return x
""" + MAIN)
syn("028_params_missing_rparen", """
import ifj26.*
def f(x: Int: Int =
  return x
""" + MAIN)
syn("029_params_missing_lparen", """
import ifj26.*
def f x: Int): Int =
  return x
""" + MAIN)
syn("030_return_type_missing_colon", """
import ifj26.*
def f() Int =
  return 1
""" + MAIN)
syn("031_return_type_colon_no_type", """
import ifj26.*
def f(): =
  return 1
""" + MAIN)
syn("032_missing_assign", """
import ifj26.*
def f(): Int
  return 1
""" + MAIN)
syn("033_body_missing_at_eof", """
import ifj26.*
@main def main(): Unit =
""")
syn("034_body_missing_next_def", """
import ifj26.*
def f(): Unit =
def g(): Unit =
  print(1)
""" + MAIN, "Blok nesmi byt prazdny.")
syn("035_body_not_indented", """
import ifj26.*
@main def main(): Unit =
print(1)
""")
syn("036_newline_after_def", """
import ifj26.*
@main def
main(): Unit =
  print(1)
""")
syn("037_newline_between_name_and_paren", """
import ifj26.*
@main def main
(): Unit =
  print(1)
""")
syn("038_newline_before_colon", """
import ifj26.*
@main def main()
: Unit =
  print(1)
""")
syn("039_newline_before_assign", """
import ifj26.*
@main def main(): Unit
=
  print(1)
""")
syn("040_newline_after_colon", """
import ifj26.*
@main def main():
Unit =
  print(1)
""")
syn("041_nested_def", """
import ifj26.*
@main def main(): Unit =
  def inner(): Unit =
    print(1)
  print(2)
""")
syn("042_statement_at_top_level", """
import ifj26.*
val x = 1
""" + MAIN)
syn("043_call_at_top_level", """
import ifj26.*
""" + MAIN + """print(1)
""")
syn("044_main_annotation_twice", """
import ifj26.*
@main @main def main(): Unit =
  print(1)
""")
syn("045_main_annotation_without_def", """
import ifj26.*
@main
val x = 1
""" + MAIN)
syn("046_main_annotation_at_eof", """
import ifj26.*
""" + MAIN + "@main\n")
syn("047_keyword_as_function_name", """
import ifj26.*
def if(): Unit =
  print(1)
""" + MAIN)
syn("048_keyword_as_param_name", """
import ifj26.*
def f(val: Int): Unit =
  print(1)
""" + MAIN)
syn("049_type_keyword_as_function_name", """
import ifj26.*
def Int(): Unit =
  print(1)
""" + MAIN)
base("30_def_without_parens_no_type", "2", "PARAMLISTS", """
import ifj26.*
def f =
  print(1)
""" + MAIN)
syn("051_param_with_default", """
import ifj26.*
def f(x: Int = 1): Int =
  return x
""" + MAIN)
syn("052_annotation_after_def", """
import ifj26.*
def @main main(): Unit =
  print(1)
""")

# --- prikazy ---
syn("060_val_without_init", """
import ifj26.*
@main def main(): Unit =
  val x
""")
syn("061_val_typed_without_init", """
import ifj26.*
@main def main(): Unit =
  val x: Int
""")
syn("062_var_missing_name", """
import ifj26.*
@main def main(): Unit =
  var = 1
""")
syn("063_val_missing_expr", """
import ifj26.*
@main def main(): Unit =
  val x =
""")
syn("064_val_missing_expr_then_stmt", """
import ifj26.*
@main def main(): Unit =
  val x =
  print(1)
""", "Za = mimo zavorky nesmi byt NL.")
syn("065_assign_missing_expr", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  x =
""")
syn("066_assign_to_literal", """
import ifj26.*
@main def main(): Unit =
  1 = 2
""")
syn("067_assign_to_expression", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  x + 1 = 2
""")
syn("068_chained_assign", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  var y = 2
  x = y = 3
""")
syn("069_two_statements_no_separator", """
import ifj26.*
@main def main(): Unit =
  val a = 1 val b = 2
""")
syn("070_double_type", """
import ifj26.*
@main def main(): Unit =
  val x: Int Int = 1
""")
syn("071_val_colon_no_type", """
import ifj26.*
@main def main(): Unit =
  val x: = 1
""")
syn("072_compound_assign", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  x += 1
""")
syn("073_return_two_exprs", """
import ifj26.*
def f(): Int =
  return 1 2
""" + MAIN)
syn("074_statement_starts_with_binop", """
import ifj26.*
@main def main(): Unit =
  * 2
""")
syn("075_unbalanced_paren_statement", """
import ifj26.*
@main def main(): Unit =
  val x = (1 + 2
  print(x)
""", "Neuzavrena zavorka pohlti NL -> 'print' uprostred vyrazu.")
syn("076_val_name_is_literal", """
import ifj26.*
@main def main(): Unit =
  val 1 = 2
""")
syn("077_val_val", """
import ifj26.*
@main def main(): Unit =
  val val x = 1
""")
syn("078_keyword_as_var_name", """
import ifj26.*
@main def main(): Unit =
  val while = 1
""")
syn("079_type_keyword_as_var_name", """
import ifj26.*
@main def main(): Unit =
  val Int = 1
""")
syn("080_return_keyword_as_var_name", """
import ifj26.*
@main def main(): Unit =
  var return = 1
""")
syn("081_val_type_on_next_line", """
import ifj26.*
@main def main(): Unit =
  val x:
    Int = 1
""")
syn("082_val_keyword_alone", """
import ifj26.*
@main def main(): Unit =
  val
""")
syn("083_semicolon_only_separator_then_eof_garbage", """
import ifj26.*
@main def main(): Unit =
  val a = 1; =
""")
syn("084_assign_with_type", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  x: Int = 2
""")
syn("085_def_keyword_as_var_name", """
import ifj26.*
@main def main(): Unit =
  val def = 1
""")
syn("086_import_inside_function", """
import ifj26.*
@main def main(): Unit =
  import ifj26.*
""")

# --- if / while ---
syn("100_if_missing_then", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0
    print(1)
  else
    print(2)
""")
syn("101_if_missing_condition", """
import ifj26.*
@main def main(): Unit =
  if then
    print(1)
  else
    print(2)
""")
syn("102_then_on_next_line", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0
  then
    print(1)
  else
    print(2)
""", "Slajd 22: mezi podminkou a then je NL zakazany.")
syn("103_else_more_indented", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    print(1)
    else
      print(2)
""", "else musi mit stejne odsazeni jako if.")
syn("104_else_less_indented", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  while x > 0 do
    if x > 0 then
      print(1)
  else
    print(2)
    x = x - 1
""", "else je na urovni while, ne if -> if je bez else a else je bez if.")
syn("105_else_without_if", """
import ifj26.*
@main def main(): Unit =
  print(1)
  else
    print(2)
""")
syn("106_empty_then_block", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
  else
    print(2)
""")
syn("107_empty_else_block", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    print(1)
  else
  print(2)
""")
syn("108_empty_else_block_at_eof", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    print(1)
  else
""")
syn("109_empty_while_block", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  while x > 0 do
  print(2)
""")
syn("110_block_only_comment", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  while x > 0 do
    // jen komentar
  print(2)
""", "Komentarovy radek netvori blok -> blok je prazdny.")
syn("111_while_missing_do", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  while x > 0
    x = x - 1
""")
syn("112_do_on_next_line", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  while x > 0
  do
    x = x - 1
""")
syn("113_while_missing_condition", """
import ifj26.*
@main def main(): Unit =
  while do
    print(1)
""")
syn("114_newline_in_condition_outside_parens", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x >
    0 then
    print(1)
  else
    print(2)
""")
syn("115_two_elses", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    print(1)
  else
    print(2)
  else
    print(3)
""")
syn("116_while_with_then", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  while x > 0 then
    x = x - 1
""")
syn("117_if_with_do", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 do
    print(1)
  else
    print(2)
""")
syn("118_else_with_then", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    print(1)
  else then
    print(2)
""")
syn("119_then_body_not_indented", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
  print(1)
  else
    print(2)
""")
syn("121_else_newline_missing_block", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    print(1)
  else
""" + "  print(3)\n", "Za else neni zvetsene odsazeni.")
syn("122_then_semicolon_body", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then; print(1)
  else
    print(2)
""")
syn("123_while_condition_is_assignment", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  while x = 0 do
    print(1)
""")
syn("124_if_paren_then_newline", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if (x > 0)
  then
    print(1)
  else
    print(2)
""")

# --- vyrazy ---
syn("140_trailing_operator", """
import ifj26.*
@main def main(): Unit =
  val x = 1 +
""")
syn("141_leading_binary_operator", """
import ifj26.*
@main def main(): Unit =
  val x = * 2
""")
syn("142_two_binary_operators", """
import ifj26.*
@main def main(): Unit =
  val x = 1 * / 2
""")
syn("143_unclosed_paren", """
import ifj26.*
@main def main(): Unit =
  val x = (1 + 2
""")
syn("144_extra_rparen", """
import ifj26.*
@main def main(): Unit =
  val x = 1 + 2)
""")
syn("145_two_operands", """
import ifj26.*
@main def main(): Unit =
  val x = 1 2
""")
syn("146_two_operands_in_parens", """
import ifj26.*
@main def main(): Unit =
  val x = (1 2)
""")
syn("147_newline_after_operator", """
import ifj26.*
@main def main(): Unit =
  var a = 3 +
    10
""", "Slajd 33: mimo zavorky je NL za operatorem zakazany.")
syn("148_missing_rparen_in_call", """
import ifj26.*
@main def main(): Unit =
  print(1
""")
syn("149_call_double_comma", """
import ifj26.*
def f(a: Int, b: Int): Int =
  return a
@main def main(): Unit =
  val r = f(1,,2)
""")
syn("150_call_leading_comma", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val r = f(,1)
""")
syn("151_call_trailing_comma_same_line", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val r = f(1,)
""", "Koncova carka vyzaduje NL pred ')'.")
syn("152_keyword_in_expression", """
import ifj26.*
@main def main(): Unit =
  val x = 1 + if
""")
syn("153_relational_missing_rhs", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x < then
    print(1)
  else
    print(2)
""")
syn("154_two_strings", """
import ifj26.*
@main def main(): Unit =
  val x = "a" "b"
""")
syn("155_empty_parens_with_operator_inside", """
import ifj26.*
@main def main(): Unit =
  val x = (+)
""")
syn("156_operator_at_end_in_parens", """
import ifj26.*
@main def main(): Unit =
  val x = (1 +)
""")
syn("157_call_missing_comma", """
import ifj26.*
def f(a: Int, b: Int): Int =
  return a
@main def main(): Unit =
  val r = f(1 2)
""")
syn("158_double_eq_as_assign", """
import ifj26.*
@main def main(): Unit =
  val x == 1
""")
syn("159_assign_in_expression", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  val y = (x = 2)
""")
syn("160_condition_ends_with_operator_before_then", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x + then
    print(1)
  else
    print(2)
""")
syn("161_number_then_ident", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  val y = 2 x
""")
syn("162_lparen_rparen_swapped", """
import ifj26.*
@main def main(): Unit =
  val x = )1 + 2(
""")
syn("163_star_slash_without_comment", """
import ifj26.*
@main def main(): Unit =
  val x = 1 */ 2
""", "'*/' mimo komentar jsou dva operatory.")
syn("164_unary_star", """
import ifj26.*
@main def main(): Unit =
  val y = 2
  val x = *y
""")
syn("165_less_greater", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x <> 2 then
    print(1)
  else
    print(2)
""")
syn("166_eq_eq_eq", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x === 2 then
    print(1)
  else
    print(2)
""")
syn("167_call_statement_extra_rparen", """
import ifj26.*
@main def main(): Unit =
  print(1))
""")
syn("168_call_args_type_annotation", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val r = f(a: Int)
""")

# =====================================================================
# SEMANTICKE CHYBY (3-10) - syntakticky spravne!
# =====================================================================
sem("01_undefined_variable", "3", """
import ifj26.*
@main def main(): Unit =
  print(x)
""")
sem("02_undefined_variable_in_expr", "3", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  val b = a + c
""")
sem("03_undefined_function", "3", """
import ifj26.*
@main def main(): Unit =
  val a = nothere(1)
""")
sem("04_use_before_definition", "3", """
import ifj26.*
@main def main(): Unit =
  print(a)
  val a = 1
""")
sem("05_out_of_scope_after_block", "3", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    val y = 2
    print(y)
  else
    print(x)
  print(y)
""")
sem("06_missing_main", "3", """
import ifj26.*
def f(): Unit =
  print(1)
""")
sem("07_only_prolog", "3", """
import ifj26.*
""", "Syntakticky OK (prazdny seznam funkci), chybi @main.")
sem("08_assign_to_undefined", "3", """
import ifj26.*
@main def main(): Unit =
  x = 1
""")
sem("09_var_of_other_function", "3", """
import ifj26.*
def f(): Unit =
  val loc = 1
  print(loc)
@main def main(): Unit =
  print(loc)
""")
sem("10_self_reference_in_init", "3", """
import ifj26.*
@main def main(): Unit =
  val a = a + 1
""")
sem("20_function_redefinition", "4", """
import ifj26.*
def f(): Unit =
  print(1)
def f(): Unit =
  print(2)
""" + MAIN)
sem("21_function_overload", "4", """
import ifj26.*
def f(): Unit =
  print(1)
def f(x: Int): Unit =
  print(x)
""" + MAIN, "Pretezovani neni povoleno.")
sem("22_var_redefinition_same_block", "4", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  var a = 2
""")
sem("23_param_redefinition_in_body", "4", """
import ifj26.*
def f(x: Int): Unit =
  val x = 0
  print(x)
""" + MAIN, "Slajd 40: 'val x = 0 // redefinice'.")
sem("24_two_mains", "4", """
import ifj26.*
@main def main(): Unit =
  print(1)
@main def main2(): Unit =
  print(2)
""")
sem("25_duplicate_param_names", "4", """
import ifj26.*
def f(a: Int, a: Int): Unit =
  print(a)
""" + MAIN)
sem("26_redefine_builtin", "4", """
import ifj26.*
def print(x: Int): Unit =
  ()
""" + MAIN, "Prekryti vestavene funkce = kolize jmen.")
sem("27_redefinition_in_nested_block", "4", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  if a > 0 then
    val b = 1
    val b = 2
  else
    ()
""")
sem("28_main_redefined_as_normal", "4", """
import ifj26.*
@main def main(): Unit =
  print(1)
def main(): Unit =
  print(2)
""")
sem("40_too_few_args", "5", """
import ifj26.*
def f(a: Int, b: Int): Int =
  return a
@main def main(): Unit =
  val r = f(1)
""")
sem("41_too_many_args", "5", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val r = f(1, 2)
""")
sem("42_wrong_arg_type", "5", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val r = f("text")
""")
sem("43_main_with_params", "5", """
import ifj26.*
@main def main(x: Int): Unit =
  print(x)
""")
sem("44_args_to_noarg_function", "5", """
import ifj26.*
def f(): Unit =
  print(1)
@main def main(): Unit =
  f(1)
""")
sem("45_builtin_wrong_arg_count", "5", """
import ifj26.*
@main def main(): Unit =
  val n = strLength()
""")
sem("46_forward_call_wrong_count", "5", """
import ifj26.*
@main def main(): Unit =
  val r = later(1, 2, 3)
def later(a: Int): Int =
  return a
""", "Volani pred definici: pocet argumentu se overi dodatecne.")
sem("47_double_to_int_param", "5", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val r = f(1.5)
""", "Konverze Double -> Int neexistuje.")
sem("60_string_plus_int", "6", """
import ifj26.*
@main def main(): Unit =
  val x = "ahoj" + 1
""", "Primo ze slajdu 6.")
sem("61_val_int_from_string", "6", """
import ifj26.*
@main def main(): Unit =
  val x: Int = "a"
""")
sem("62_val_int_from_double", "6", """
import ifj26.*
@main def main(): Unit =
  val x: Int = 1.5
""")
sem("63_assign_string_to_int_var", "6", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  x = "s"
""")
sem("64_return_wrong_type", "6", """
import ifj26.*
def f(): Int =
  return "s"
""" + MAIN)
sem("65_main_returns_int", "6", """
import ifj26.*
@main def main(): Int =
  return 1
""")
sem("66_compare_string_int", "6", """
import ifj26.*
@main def main(): Unit =
  val s = "a"
  if s < 1 then
    print(1)
  else
    print(2)
""")
sem("67_string_times_int", "6", """
import ifj26.*
@main def main(): Unit =
  val s = "a" * 3
""")
sem("68_unary_minus_string", "6", """
import ifj26.*
@main def main(): Unit =
  val s = -"a"
""")
sem("69_implicit_return_wrong_type", "6", """
import ifj26.*
def f(): Int =
  "s"
""" + MAIN)
sem("70_condition_not_boolean", "6", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x then
    print(1)
  else
    print(2)
""")
sem("71_assign_double_to_int_var", "6", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  x = 2.5
""")
sem("72_call_result_wrong_type", "6", """
import ifj26.*
def f(): String =
  return "s"
@main def main(): Unit =
  val x: Int = f()
""")
sem("73_string_minus_string", "6", """
import ifj26.*
@main def main(): Unit =
  val x = "a" - "b"
""")
sem("74_eq_string_double", "6", """
import ifj26.*
@main def main(): Unit =
  val s = "a"
  if s == 1.0 then
    print(1)
  else
    print(2)
""")
sem("75_unit_in_arithmetic", "6", """
import ifj26.*
def f(): Unit =
  ()
@main def main(): Unit =
  val u = f()
  val x = u + 1
""", "Unit nelze scitat.")
sem("90_assign_to_val", "7", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  x = 2
""")
sem("91_assign_to_param", "7", """
import ifj26.*
def f(x: Int): Unit =
  x = 1
""" + MAIN, "Slajd 40: 'x = 1 // parametr je val'.")
sem("92_assign_to_val_in_nested_block", "7", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  while x > 0 do
    x = x - 1
""")
sem("93_assign_call_to_val", "7", """
import ifj26.*
def f(): Int =
  return 1
@main def main(): Unit =
  val x = 0
  x = f()
""")

# =====================================================================
# BASE_ONLY: v zakladu chyba, s rozsirenim platne
# =====================================================================
base("01_call_in_expression", "2", "FUNEXP", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val x = f(1) + 2
""")
base("02_expression_as_argument", "2", "FUNEXP", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val y = 1
  val x = f(y + 1)
""", "Slajd 20: 'nelze pouzit vyraz jako argument volani'.")
base("03_nested_call", "2", "FUNEXP", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val x = f(f(1))
""")
base("04_call_in_condition", "2", "FUNEXP", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  if f(1) > 0 then
    print(1)
  else
    print(2)
""")
base("05_call_statement_plus", "2", "FUNEXP", """
import ifj26.*
@main def main(): Unit =
  print(1) + 1
""")
base("06_call_parenthesized_in_expr", "2", "FUNEXP", """
import ifj26.*
def f(): Int =
  return 1
@main def main(): Unit =
  val x = (f())
""")
base("07_else_if_same_line", "2", "BOOLTHEN", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x < 0 then
    print(1)
  else if x == 0 then
    print(2)
  else
    print(3)
""")
base("08_if_without_else", "2", "BOOLTHEN/CONTROL", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    print(1)
  print(2)
""", "Zadani: v zakladu je else povinne.")
base("09_then_body_same_line", "2", "CONTROL", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then print(1)
  else
    print(2)
""")
base("10_else_body_same_line", "2", "CONTROL", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  if x > 0 then
    print(1)
  else print(2)
""")
base("11_while_body_same_line", "2", "CONTROL", """
import ifj26.*
@main def main(): Unit =
  var x = 1
  while x > 0 do x = x - 1
""")
base("12_if_as_expression", "2", "CONTROL", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  val a = if x < 0 then 0 - x else x
""")
base("13_logical_and", "1 2", "BOOLTHEN", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  if a < 1 && a > 0 then
    print(1)
  else
    print(2)
""", "V zakladu '&&' neni lexem -> 1, pokud ho scanner zna -> 2.")
base("14_logical_not", "1 2", "BOOLTHEN", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  if !(a < 1) then
    print(1)
  else
    print(2)
""")
base("15_true_literal", "2 3", "BOOLTHEN", """
import ifj26.*
@main def main(): Unit =
  val b = true
""", "Bez BOOLTHEN je 'true' identifikator -> nedefinovana promenna (3), nebo rezervovane slovo (2).")
base("16_boolean_type", "2", "BOOLTHEN", """
import ifj26.*
def f(b: Boolean): Unit =
  print(1)
""" + MAIN)
base("17_bool_into_variable", "2 6", "BOOLTHEN", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  val b = a < 2
""", "Syntakticky je to OK vyraz; semanticky Boolean nelze bez BOOLTHEN ukladat.")
base("18_member_access", "2", "VALMEM", """
import ifj26.*
@main def main(): Unit =
  val a = 5
  val b = a.toString
""")
base("19_def_without_parens", "2", "PARAMLISTS", """
import ifj26.*
def answer: Int =
  return 42
""" + MAIN)
base("20_multiple_param_lists", "2", "PARAMLISTS", """
import ifj26.*
def add(a: Int)(b: Int): Int =
  return a + b
""" + MAIN)
base("21_for_loop", "2", "CONTROL", """
import ifj26.*
@main def main(): Unit =
  for i <- 1 to 10 do
    print(i)
""", "for je rezervovane klicove slovo.")
base("22_match", "2", "ALGEBDT", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  val s = x match
    case 0 => "zero"
    case _ => "other"
""")
base("23_interpolated_string", "1 2", "INTERPOLATEDS", """
import ifj26.*
@main def main(): Unit =
  val n = 1
  print(s"n = $n")
""")
base("24_lambda", "2", "LAMBDAS", """
import ifj26.*
@main def main(): Unit =
  val f = (n: Int) => n + 1
""")
base("25_list", "1 2", "LISTS", """
import ifj26.*
@main def main(): Unit =
  val l = List(1, 2, 3)
""")
base("26_type_alias", "2", "ALGEBDT", """
import ifj26.*
type Scalar = Int | Double
""" + MAIN)
base("27_extension", "2", "EXTKW", """
import ifj26.*
extension (value: Int)
  def twice: Int = value * 2
""" + MAIN)
base("28_union_param_type", "1 2", "ALGEBDT", """
import ifj26.*
def foo(x: Double | String): Unit =
  print(1)
""" + MAIN)
base("29_return_call_expression", "2", "FUNEXP", """
import ifj26.*
def f(a: Int): Int =
  return a
def g(a: Int): Int =
  return f(a) + 1
""" + MAIN)

# =====================================================================
# UNCLEAR: zadani/demo to jednoznacne nerika (povolene kody v .code)
# =====================================================================
unclear("01_import_with_spaces", "0 2", """
import ifj26 . *
@main def main(): Unit =
  print(1)
""", "Slajd 22: 'import? ifj26?.?*' - mezery v prologu nejsou jednoznacne. Over na foru.")
unclear("02_single_line_function_body", "0 2", """
import ifj26.*
def f(): Int = 5
@main def main(): Unit =
  print(1)
""", "V zakladu je za '=' v hlavicce <NL+> (slajd 22); telo na stejnem radku jen v rozsirenich (slajd 56).")
unclear("03_trailing_semicolon", "0 2", """
import ifj26.*
@main def main(): Unit =
  print(1);
  print(2)
""", "Slajd 22: ';?' - strednik na konci radku neni jednoznacny. Scala to povoli.")
unclear("04_double_semicolon", "0 2", """
import ifj26.*
@main def main(): Unit =
  print(1);; print(2)
""", "Prazdny prikaz mezi stredniky.")
unclear("05_return_without_value", "0 2", """
import ifj26.*
def f(): Unit =
  print(1)
  return
""" + MAIN, "Scala povoluje 'return' bez hodnoty v Unit funkci.")
unclear("06_return_call", "0 2", """
import ifj26.*
def v(x: Int): Int =
  return x
def w(x: Int): Int =
  return v(x)
""" + MAIN, "Je volani povoleno jako navratova hodnota return? (prirazeni volani ano)")
unclear("07_negative_literal_argument", "0 2", """
import ifj26.*
def f(a: Int): Int =
  return a
@main def main(): Unit =
  val r = f(-1)
""", "-1 je unarni minus + literal = vyraz. Vyraz jako argument je v zakladu zakazany.")
unclear("08_continuation_less_indented", "0 1 2", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  val z = (x +
1)
  print(z)
""", "Slajd 26: pokracovani musi byt odsazeno vic nez prvni fyzicky radek logickeho radku.")
unclear("09_unexpected_indent", "1 2", """
import ifj26.*
@main def main(): Unit =
  var x = 1
    x = 2
""", "Vetsi odsazeni bez hlavicky bloku. Chyba urcite, kod 1 nebo 2 dle foru.")
unclear("10_dedent_mismatch", "1 2", """
import ifj26.*
@main def main(): Unit =
    var x = 1
    while x > 0 do
        x = x - 1
      print(x)
""", "Navrat na odsazeni 6, ktere neni na zasobniku [0,4,8].")
unclear("11_def_indented_at_top", "1 2", """
import ifj26.*
  @main def main(): Unit =
    print(1)
""", "Odsazena definice funkce na nejvyssi urovni.")
unclear("12_relational_chain", "2 6", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  val b = 2
  val c = 3
  if a < b < c then
    print(1)
  else
    print(2)
""", "Relacni operatory jsou bez asociativity -> v precedencni tabulce prazdna bunka = 2. Nejspis 2.")
unclear("13_equality_chain", "2 6", """
import ifj26.*
@main def main(): Unit =
  val a = 1
  if a == 1 != 2 then
    print(1)
  else
    print(2)
""", "Stejne jako relacni retez. Nejspis 2.")
unclear("14_unknown_type_name", "2 3", """
import ifj26.*
def f(x: Float): Unit =
  print(1)
""" + MAIN, "Typy jsou v zakladu klicova slova -> nejspis 2.")
unclear("15_at_without_main", "1 2", """
import ifj26.*
@foo def main(): Unit =
  print(1)
""")
unclear("16_number_followed_by_ident", "1 2", """
import ifj26.*
@main def main(): Unit =
  val x = 10abc
""", "Slajd 21: zravy lexer -> 10 a abc (pak 2), nebo chybny literal (1).")
unclear("17_double_dot_number", "1 2", """
import ifj26.*
@main def main(): Unit =
  val x = 1.2.3
""")
unclear("18_leading_zeros", "0 1", """
import ifj26.*
@main def main(): Unit =
  val x = 007
""", "Ve Scale jsou uvodni nuly zakazane. Zadani - over.")
unclear("19_double_suffix", "0 1 2", """
import ifj26.*
@main def main(): Unit =
  val a = 2d
  val b = 2.5D
""", "Sufix d/D u Double - over v zadani (sekce o literalech).")
unclear("20_param_trailing_comma_with_newline", "0 2", """
import ifj26.*
def f(
  a: Int,
): Int =
  return a
""" + MAIN, "U argumentu je to na slajdu 22, u parametru ne.")
unclear("21_dot_fraction", "0 1 2", """
import ifj26.*
@main def main(): Unit =
  val a = .5
  val b = 1.
""")
unclear("22_expression_statement", "0", """
import ifj26.*
@main def main(): Unit =
  val x = 1
  x + 1
  print(x)
""", "Syntakticky vyraz jako prikaz projde (implicitni return); uprostred bloku by mel byt jen zahozen.")
unclear("23_int_overflow", "0 1", """
import ifj26.*
@main def main(): Unit =
  val x = 2147483648
""", "Int je 32bitovy.")
unclear("24_block_comment_newline_then_code_same_line", "0 1 2", """
import ifj26.*
@main def main(): Unit =
  val a = 1 /* komentar
  */ val b = 2
  print(a)
""", "Slajd 24: blokovy komentar s NL = <NL> se specialnim pravidlem.")
unclear("25_string_with_non_ascii", "0 1", """
import ifj26.*
@main def main(): Unit =
  print("Příliš žluťoučký kůň")
""", "String je jen ASCII.")
unclear("26_unit_literal_argument", "0 2", """
import ifj26.*
def f(u: Unit): Unit =
  print(1)
@main def main(): Unit =
  f(())
""")
unclear("27_reserved_keyword_identifier", "0 2", """
import ifj26.*
@main def main(): Unit =
  val match = 1
  val case = 2
""", "Klicova slova rozsireni jsou rezervovana i v zakladu? Nejspis 2.")
unclear("28_to_until_by_identifiers", "0", """
import ifj26.*
@main def main(): Unit =
  val to = 1
  val until = 2
  val by = 3
  print(to)
""", "Fórum: to/until/by NEjsou klicova slova -> mely by byt platne identifikatory (over).")


def main():
    for d, name, codes, src, note in TESTS:
        p = os.path.join(OUT, d)
        os.makedirs(p, exist_ok=True)
        with open(os.path.join(p, name + ".ifj26"), "w", newline="") as f:
            f.write(src)
        with open(os.path.join(p, name + ".code"), "w") as f:
            f.write(codes + "\n")
            if note:
                f.write("# " + note + "\n")
    names = [(d, n) for d, n, *_ in TESTS]
    assert len(names) == len(set(names)), "duplicitni nazev testu"
    from collections import Counter
    c = Counter(d for d, *_ in TESTS)
    print(dict(c), "celkem", len(TESTS))


main()
