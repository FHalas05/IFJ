# Testy parseru IFJ26

Testy vycházejí z dema k projektu (slajdy 20–27) a ze Scaly 3, ne z implementace.

| Složka | Očekávaný kód | Počet | Co testuje |
|---|---|---|---|
| `ok/` | 0 | 25 | správné programy: bloky, odsazení, komentáře, prázdné řádky, výrazy, volání, newline v závorkách |
| `err_syntax/` | 2 | 60 | syntaktické chyby: prolog, hlavička funkce, bloky, odsazení, výrazy, volání, oddělovače |
| `err_lex/` | 1 | 12 | lexikální chyby: tabulátor, řetězce, čísla, komentáře, neplatné znaky |
| `spec/` | ? | 17 | věci, které demo jednoznačně neříká; očekávání je v komentáři na 1. řádku souboru |

## Spuštění

```bash
chmod +x tests/parser/run_tests.sh
./tests/parser/run_tests.sh ./ifj26
```

Argument je cesta k přeloženému překladači (čte program ze stdin).

## Známé věci (stav k 28. 10.)

- `ok/02_demo_program` a `ok/13_multiline_parens`: nové řádky uvnitř závorek, projdou až to bude řešit scanner.
- `ok/10_comments`: blokový komentář přes víc řádků na samostatném řádku vrací navíc `T_NEWLINE`
  (scanner.c, `FSM_comments`, stav F6 nekontroluje `last_token_type`).
- Testy sémantiky (nedefinované proměnné, typy, chybějící `@main`) tu nejsou – parser bez sémantiky vrací 0.
