// Petit générateur de fichiers Excel (.xlsx) sans bibliothèque externe.
// Un .xlsx est une archive ZIP de quelques fichiers XML : on les écrit ici
// en mode « stocké » (sans compression), ce qu'Excel accepte très bien.
// Tout est en C++ standard, chaînes en UTF-8.
#ifndef XLSXWRITER_H
#define XLSXWRITER_H

#include <cstdint>
#include <string>
#include <vector>

namespace xlsx {

struct Cell {
    std::string text;      // texte de la cellule (UTF-8)
    bool isNumber = false; // true -> écrit comme nombre
};

using Row = std::vector<Cell>;

inline Cell text(const std::string &s) { return Cell{s, false}; }
inline Cell number(long long v) { return Cell{std::to_string(v), true}; }

namespace detail {

inline uint32_t crc32(const std::string &data) {
    static uint32_t table[256];
    static bool ready = false;
    if (!ready) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        ready = true;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char ch : data) crc = table[(crc ^ ch) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

inline void put16(std::string &out, uint16_t v) {
    out += char(v & 0xFF);
    out += char((v >> 8) & 0xFF);
}

inline void put32(std::string &out, uint32_t v) {
    for (int i = 0; i < 4; ++i) out += char((v >> (8 * i)) & 0xFF);
}

struct Entry {
    std::string name, data;
};

// Archive ZIP « stockée » (méthode 0), suffisante pour un .xlsx.
inline std::string zip(const std::vector<Entry> &entries) {
    std::string out, central;
    for (const Entry &e : entries) {
        const uint32_t offset = uint32_t(out.size());
        const uint32_t crc = crc32(e.data);
        const uint32_t size = uint32_t(e.data.size());
        // en-tête local
        put32(out, 0x04034b50); put16(out, 20); put16(out, 0); put16(out, 0);
        put16(out, 0); put16(out, 0x21);  // heure, date (1980-01-01)
        put32(out, crc); put32(out, size); put32(out, size);
        put16(out, uint16_t(e.name.size())); put16(out, 0);
        out += e.name;
        out += e.data;
        // entrée du répertoire central
        put32(central, 0x02014b50); put16(central, 20); put16(central, 20);
        put16(central, 0); put16(central, 0); put16(central, 0); put16(central, 0x21);
        put32(central, crc); put32(central, size); put32(central, size);
        put16(central, uint16_t(e.name.size())); put16(central, 0); put16(central, 0);
        put16(central, 0); put16(central, 0); put32(central, 0); put32(central, offset);
        central += e.name;
    }
    const uint32_t centralOffset = uint32_t(out.size());
    out += central;
    put32(out, 0x06054b50); put16(out, 0); put16(out, 0);
    put16(out, uint16_t(entries.size())); put16(out, uint16_t(entries.size()));
    put32(out, uint32_t(central.size())); put32(out, centralOffset); put16(out, 0);
    return out;
}

inline std::string escape(const std::string &s) {
    std::string r;
    for (char c : s) {
        switch (c) {
        case '&': r += "&amp;"; break;
        case '<': r += "&lt;"; break;
        case '>': r += "&gt;"; break;
        case '"': r += "&quot;"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n') break;  // interdit en XML
            r += c;
        }
    }
    return r;
}

inline std::string columnName(size_t index) {
    std::string name;
    size_t n = index + 1;
    while (n > 0) {
        name.insert(name.begin(), char('A' + (n - 1) % 26));
        n = (n - 1) / 26;
    }
    return name;
}

}  // namespace detail

// Construit le contenu binaire d'un classeur .xlsx d'une seule feuille.
// La première ligne de « rows » est l'en-tête : en gras, blanc sur bleu, figée.
// « widths » = largeur de chaque colonne (en caractères).
inline std::string build(const std::string &sheetName, const std::vector<Row> &rows,
                         const std::vector<double> &widths) {
    using namespace detail;
    const std::string xmlHead = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

    std::string sheet = xmlHead +
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<sheetViews><sheetView workbookViewId=\"0\">"
        "<pane ySplit=\"1\" topLeftCell=\"A2\" activePane=\"bottomLeft\" state=\"frozen\"/>"
        "</sheetView></sheetViews>";
    if (!widths.empty()) {
        sheet += "<cols>";
        for (size_t i = 0; i < widths.size(); ++i) {
            const std::string n = std::to_string(i + 1);
            sheet += "<col min=\"" + n + "\" max=\"" + n + "\" width=\"" + std::to_string(widths[i]) +
                     "\" customWidth=\"1\"/>";
        }
        sheet += "</cols>";
    }
    sheet += "<sheetData>";
    for (size_t r = 0; r < rows.size(); ++r) {
        const std::string rowNum = std::to_string(r + 1);
        sheet += "<row r=\"" + rowNum + "\"" + (r == 0 ? " ht=\"22\" customHeight=\"1\"" : "") + ">";
        for (size_t c = 0; c < rows[r].size(); ++c) {
            const Cell &cell = rows[r][c];
            const std::string ref = columnName(c) + rowNum;
            const std::string style = r == 0 ? " s=\"1\"" : " s=\"2\"";
            if (cell.isNumber)
                sheet += "<c r=\"" + ref + "\"" + style + "><v>" + escape(cell.text) + "</v></c>";
            else
                sheet += "<c r=\"" + ref + "\"" + style + " t=\"inlineStr\"><is><t xml:space=\"preserve\">" +
                         escape(cell.text) + "</t></is></c>";
        }
        sheet += "</row>";
    }
    sheet += "</sheetData>";
    if (rows.size() > 1 && !rows[0].empty())
        sheet += "<autoFilter ref=\"A1:" + columnName(rows[0].size() - 1) + std::to_string(rows.size()) + "\"/>";
    sheet += "</worksheet>";

    const std::string styles = xmlHead +
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<fonts count=\"2\">"
        "<font><sz val=\"11\"/><name val=\"Calibri\"/></font>"
        "<font><b/><sz val=\"11\"/><color rgb=\"FFFFFFFF\"/><name val=\"Calibri\"/></font>"
        "</fonts>"
        "<fills count=\"3\">"
        "<fill><patternFill patternType=\"none\"/></fill>"
        "<fill><patternFill patternType=\"gray125\"/></fill>"
        "<fill><patternFill patternType=\"solid\"><fgColor rgb=\"FF1665E8\"/><bgColor indexed=\"64\"/></patternFill></fill>"
        "</fills>"
        "<borders count=\"2\">"
        "<border><left/><right/><top/><bottom/><diagonal/></border>"
        "<border><left style=\"thin\"><color rgb=\"FFD5DFEE\"/></left><right style=\"thin\"><color rgb=\"FFD5DFEE\"/></right>"
        "<top style=\"thin\"><color rgb=\"FFD5DFEE\"/></top><bottom style=\"thin\"><color rgb=\"FFD5DFEE\"/></bottom><diagonal/></border>"
        "</borders>"
        "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
        "<cellXfs count=\"3\">"
        "<xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>"
        "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"2\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyFill=\"1\" applyBorder=\"1\" applyAlignment=\"1\">"
        "<alignment vertical=\"center\"/></xf>"
        "<xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"1\" xfId=\"0\" applyBorder=\"1\" applyAlignment=\"1\">"
        "<alignment vertical=\"center\"/></xf>"
        "</cellXfs>"
        "<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles>"
        "</styleSheet>";

    const std::string workbook = xmlHead +
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets><sheet name=\"" + escape(sheetName) + "\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
        "</workbook>";

    const std::string workbookRels = xmlHead +
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
        "</Relationships>";

    const std::string rootRels = xmlHead +
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
        "</Relationships>";

    const std::string contentTypes = xmlHead +
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
        "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
        "</Types>";

    return zip({{"[Content_Types].xml", contentTypes},
                {"_rels/.rels", rootRels},
                {"xl/workbook.xml", workbook},
                {"xl/_rels/workbook.xml.rels", workbookRels},
                {"xl/styles.xml", styles},
                {"xl/worksheets/sheet1.xml", sheet}});
}

}  // namespace xlsx

#endif // XLSXWRITER_H
