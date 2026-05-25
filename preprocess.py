"""
Препроцессинг датасета Stack Overflow для инвертированного индекса.

Читает Questions.csv, токенизирует текст (title + body),
сохраняет результат в JSONL-файл.

Запуск:
    python preprocess.py --input data/Questions.csv --output data/processed/docs.jsonl
    python preprocess.py --input data/Questions.csv --output data/processed/docs.jsonl --limit 50000
"""
# Сори если не работает я тестил на mock данных :)

import argparse
import csv
import html
import json
import re
import sys
from pathlib import Path

STOP_WORDS = {
    "the", "and", "a", "of", "to", "in", "is", "for", "that", "on", "with",
    "as", "it", "an", "by", "are", "be", "this", "from", "at", "or", "how"
}

def clean_html(raw_html: str) -> str:
    if not raw_html:
        return ""
    clean_text = re.sub(r'<[^>]+>', ' ', raw_html)
    return html.unescape(clean_text)


def tokenize(text: str) -> list[str]:
    text = text.lower()
    text = re.sub(r"[^a-z0-9+#\s]", " ", text)
    tokens = []
    for word in text.split():
        if len(word) > 1 and word not in STOP_WORDS:
            tokens.append(word)
    return tokens


def preprocess(input_path: Path, output_path: Path, limit: int | None) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    csv.field_size_limit(sys.maxsize)
    count = 0
    with open(input_path, encoding="utf-8", errors="replace") as f, open(output_path, "w", encoding="utf-8") as out:
        reader = csv.DictReader(f)
        for row in reader:
            if limit is not None and count >= limit:
                break
            doc_id_str = row.get("Id", "")
            if not doc_id_str.isdigit():
                continue
            doc_id = row.get("Id", "")
            title = row.get("Title", "")
            body = row.get("Body", "")
            clean_body = clean_html(body)
            tokens = tokenize(title + " " + clean_body)
            if not tokens:
                continue
            clean_title = title.replace("\n", " ").replace("\r", " ").replace("\t", " ")
            out.write(json.dumps({"doc_id": doc_id, "title": clean_title, "tokens": tokens}, ensure_ascii=False) + "\n")
            count += 1

            if count % 10_000 == 0:
                print(f"Обработано: {count} документов", file=sys.stderr)

    print(f"Готово. Всего документов: {count}", file=sys.stderr)
    print(f"Результат: {output_path}", file=sys.stderr)


def main() -> None:
    parser = argparse.ArgumentParser(description="Препроцессинг Stack Overflow датасета")
    parser.add_argument("--input", required=True, type=Path, help="Путь к Questions.csv")
    parser.add_argument("--output", required=True, type=Path, help="Путь к выходному JSONL-файлу")
    parser.add_argument("--limit", type=int, default=None, help="Максимальное количество документов")
    args = parser.parse_args()

    if not args.input.exists():
        print(f"Ошибка: файл не найден: {args.input}", file=sys.stderr)
        sys.exit(1)

    preprocess(args.input, args.output, args.limit)


if __name__ == "__main__":
    main()
