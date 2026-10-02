#include "portable_setup.h"
#include "asset_setup.h"
#include "asset_dlc.h"
#include "asset_hash.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <future>
#include <atomic>
#include <commctrl.h>
#include <shlobj.h>

namespace iu::portable {
namespace {
fs::path home;
bool es = false;
const std::pair<std::string,std::string> strings[] = {
#include "setup_strings.inc"
};

void Replace(std::string& s, const std::string& a, const std::string& b) {
  size_t i = 0; while ((i = s.find(a, i)) != std::string::npos) { s.replace(i, a.size(), b); i += b.size(); }
}

std::string Utf8(const std::wstring& s) {
  int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
  std::string out(n, 0); WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), out.data(), n, nullptr, nullptr); return out;
}

void NoLinks(const fs::path& path) {
  for (auto p = fs::absolute(path); !p.empty();) {
    auto a = GetFileAttributesW(p.c_str());
    if (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_REPARSE_POINT))
      throw std::runtime_error(Text("Ruta portable invalida."));
    auto up = p.parent_path(); if (up == p) break; p = up;
  }
}

void WriteConfig(const fs::path& file) {
  NoLinks(file); fs::create_directories(file.parent_path());
  auto temp = file; temp += L".tmp"; NoLinks(temp);
  std::ofstream out(temp, std::ios::binary | std::ios::trunc);
  out << "{\n  \"language\": \"" << (es ? "es" : "en") << "\",\n  \"portable\": true,\n  \"schema\": 1\n}\n";
  out.close();
  if (!out || !MoveFileExW(temp.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error(Text("No se pueden guardar las preferencias."));
}

void Fill(HDC dc, RECT r, COLORREF c) {
  auto b = CreateSolidBrush(c); FillRect(dc, &r, b); DeleteObject(b);
}

void DrawBorder(HDC dc, RECT r, COLORREF c, int width = 1) {
  auto pen = CreatePen(PS_SOLID, width, c);
  auto old_pen = SelectObject(dc, pen);
  auto old_brush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
  Rectangle(dc, r.left, r.top, r.right, r.bottom);
  SelectObject(dc, old_brush);
  SelectObject(dc, old_pen);
  DeleteObject(pen);
}

void DrawRoundRect(HDC dc, RECT r, int radius, COLORREF border, COLORREF fill) {
  auto pen = CreatePen(PS_SOLID, 1, border);
  auto brush = CreateSolidBrush(fill);
  auto old_pen = SelectObject(dc, pen);
  auto old_brush = SelectObject(dc, brush);
  RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
  SelectObject(dc, old_brush);
  SelectObject(dc, old_pen);
  DeleteObject(brush);
  DeleteObject(pen);
}

void Label(HDC dc, HFONT font, RECT r, const std::wstring& s, COLORREF color, UINT flags = DT_LEFT | DT_WORDBREAK) {
  SelectObject(dc, font);
  SetTextColor(dc, color);
  SetBkMode(dc, TRANSPARENT);
  DrawTextW(dc, s.c_str(), -1, &r, flags);
}

// Procedural 8-pointed silver compass crest star
void DrawCrest(HDC dc, int cx, int cy, int radius) {
  COLORREF silver = RGB(225, 235, 255);
  COLORREF steel = RGB(130, 160, 205);
  auto pen = CreatePen(PS_SOLID, 1, silver);
  auto old_pen = SelectObject(dc, pen);

  // Outer rays
  for (int angle = 0; angle < 4; ++angle) {
    double rad = angle * 3.14159265 / 2.0;
    int x1 = cx + int(std::cos(rad) * radius);
    int y1 = cy + int(std::sin(rad) * radius);
    int x2 = cx - int(std::cos(rad) * radius);
    int y2 = cy - int(std::sin(rad) * radius);
    MoveToEx(dc, x1, y1, nullptr);
    LineTo(dc, x2, y2);
  }
  // Minor rays
  int r_minor = radius * 6 / 10;
  for (int angle = 0; angle < 4; ++angle) {
    double rad = (angle * 3.14159265 / 2.0) + (3.14159265 / 4.0);
    int x1 = cx + int(std::cos(rad) * r_minor);
    int y1 = cy + int(std::sin(rad) * r_minor);
    int x2 = cx - int(std::cos(rad) * r_minor);
    int y2 = cy - int(std::sin(rad) * r_minor);
    MoveToEx(dc, x1, y1, nullptr);
    LineTo(dc, x2, y2);
  }
  // Central diamond
  POINT pts[] = {
    {cx, cy - radius / 3},
    {cx + radius / 3, cy},
    {cx, cy + radius / 3},
    {cx - radius / 3, cy}
  };
  auto b = CreateSolidBrush(steel);
  auto old_b = SelectObject(dc, b);
  Polygon(dc, pts, 4);
  SelectObject(dc, old_b);
  DeleteObject(b);

  SelectObject(dc, old_pen);
  DeleteObject(pen);
}

// Procedural metallic optical disc icon with specular reflection
void DrawDiscIcon(HDC dc, int cx, int cy, int radius) {
  // Outer disc gradient simulation
  for (int r = radius; r >= radius - 3; --r) {
    auto b = CreateSolidBrush(RGB(175 - (radius - r) * 15, 195 - (radius - r) * 15, 220 - (radius - r) * 15));
    auto old_b = SelectObject(dc, b);
    auto old_p = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(dc, old_p);
    SelectObject(dc, old_b);
    DeleteObject(b);
  }
  // Data tracks
  int mid = radius - 3;
  auto track_b = CreateSolidBrush(RGB(110, 138, 172));
  auto old_b = SelectObject(dc, track_b);
  auto old_p = SelectObject(dc, GetStockObject(NULL_PEN));
  Ellipse(dc, cx - mid, cy - mid, cx + mid, cy + mid);
  SelectObject(dc, old_p);
  SelectObject(dc, old_b);
  DeleteObject(track_b);

  // Specular sheen wedge
  POINT wedge[] = {
    {cx, cy},
    {cx + mid * 7 / 10, cy - mid * 7 / 10},
    {cx + mid * 9 / 10, cy - mid * 4 / 10}
  };
  auto sheen_b = CreateSolidBrush(RGB(220, 235, 255));
  old_b = SelectObject(dc, sheen_b);
  old_p = SelectObject(dc, GetStockObject(NULL_PEN));
  Polygon(dc, wedge, 3);
  SelectObject(dc, old_p);
  SelectObject(dc, old_b);
  DeleteObject(sheen_b);

  // Inner hub silver ring
  int hub = radius * 4 / 10;
  auto hub_b = CreateSolidBrush(RGB(200, 215, 235));
  old_b = SelectObject(dc, hub_b);
  old_p = SelectObject(dc, GetStockObject(NULL_PEN));
  Ellipse(dc, cx - hub, cy - hub, cx + hub, cy + hub);
  SelectObject(dc, old_p);
  SelectObject(dc, old_b);
  DeleteObject(hub_b);

  // Center spindle hole
  int hole = radius * 2 / 10;
  auto hole_b = CreateSolidBrush(RGB(15, 23, 38));
  old_b = SelectObject(dc, hole_b);
  old_p = SelectObject(dc, GetStockObject(NULL_PEN));
  Ellipse(dc, cx - hole, cy - hole, cx + hole, cy + hole);
  SelectObject(dc, old_p);
  SelectObject(dc, old_b);
  DeleteObject(hole_b);
}

// Left Decorative Card 1: Moonlit Gothic Cathedral
void DrawCard1(HDC dc, RECT r) {
  int w = r.right - r.left, h = r.bottom - r.top;
  // Sky vertical gradient
  for (int y = 0; y < h; ++y) {
    int g = 14 + y * 20 / h;
    int b = 28 + y * 35 / h;
    Fill(dc, {r.left, r.top + y, r.right, r.top + y + 1}, RGB(10, g, b));
  }
  // Soft glowing full moon
  int mx = r.left + w * 7 / 10, my = r.top + h * 35 / 100, mr = 20;
  auto halo = CreateSolidBrush(RGB(45, 65, 95));
  auto old_b = SelectObject(dc, halo);
  auto old_p = SelectObject(dc, GetStockObject(NULL_PEN));
  Ellipse(dc, mx - mr - 8, my - mr - 8, mx + mr + 8, my + mr + 8);
  SelectObject(dc, old_b);
  DeleteObject(halo);

  auto moon = CreateSolidBrush(RGB(230, 238, 250));
  old_b = SelectObject(dc, moon);
  Ellipse(dc, mx - mr, my - mr, mx + mr, my + mr);
  SelectObject(dc, old_b);
  DeleteObject(moon);

  // Gothic cathedral spires silhouette
  POINT spires[] = {
    {r.left, r.bottom},
    {r.left, r.top + h * 55 / 100},
    {r.left + w * 18 / 100, r.top + h * 20 / 100},
    {r.left + w * 28 / 100, r.top + h * 50 / 100},
    {r.left + w * 42 / 100, r.top + h * 15 / 100},
    {r.left + w * 55 / 100, r.top + h * 55 / 100},
    {r.left + w * 75 / 100, r.top + h * 30 / 100},
    {r.left + w * 88 / 100, r.top + h * 60 / 100},
    {r.right, r.top + h * 62 / 100},
    {r.right, r.bottom}
  };
  auto spire_b = CreateSolidBrush(RGB(14, 20, 34));
  old_b = SelectObject(dc, spire_b);
  Polygon(dc, spires, 10);
  SelectObject(dc, old_b);
  DeleteObject(spire_b);

  // Illuminated warm candlelit gothic windows
  for (int i = 0; i < 3; ++i) {
    int wx = r.left + 28 + i * 42;
    int wy = r.top + h * 60 / 100;
    auto win_b = CreateSolidBrush(RGB(245, 215, 130));
    old_b = SelectObject(dc, win_b);
    RoundRect(dc, wx, wy, wx + 12, wy + 26, 8, 8);
    SelectObject(dc, old_b);
    DeleteObject(win_b);
  }
  // Stone bridge / water reflection base
  Fill(dc, {r.left, r.bottom - 18, r.right, r.bottom}, RGB(8, 14, 24));
  SelectObject(dc, old_p);
  DrawBorder(dc, r, RGB(55, 80, 115));
}

// Left Decorative Card 2: Fortress Bridge with Azure Banners
void DrawCard2(HDC dc, RECT r) {
  int w = r.right - r.left, h = r.bottom - r.top;
  for (int y = 0; y < h; ++y) {
    int g = 16 + y * 24 / h;
    int b = 30 + y * 40 / h;
    Fill(dc, {r.left, r.top + y, r.right, r.top + y + 1}, RGB(11, g, b));
  }
  auto old_p = SelectObject(dc, GetStockObject(NULL_PEN));

  // Stone bridge arches
  for (int i = 0; i < 3; ++i) {
    int ax = r.left + 15 + i * 65;
    int ay = r.top + h * 50 / 100;
    auto arch_b = CreateSolidBrush(RGB(18, 26, 42));
    auto old_b = SelectObject(dc, arch_b);
    Rectangle(dc, ax, ay, ax + 55, r.bottom);
    SelectObject(dc, old_b);
    DeleteObject(arch_b);

    auto cut_b = CreateSolidBrush(RGB(10, 16, 28));
    old_b = SelectObject(dc, cut_b);
    Ellipse(dc, ax + 8, ay + 15, ax + 47, r.bottom + 15);
    SelectObject(dc, old_b);
    DeleteObject(cut_b);
  }

  // Regal royal blue banners hanging from bridge
  for (int i = 0; i < 3; ++i) {
    int bx = r.left + 35 + i * 65;
    int by = r.top + h * 42 / 100;
    POINT banner[] = {
      {bx, by},
      {bx + 14, by},
      {bx + 14, by + 34},
      {bx + 7, by + 40},
      {bx, by + 34}
    };
    auto banner_b = CreateSolidBrush(RGB(25, 65, 145));
    auto old_b = SelectObject(dc, banner_b);
    Polygon(dc, banner, 5);
    SelectObject(dc, old_b);
    DeleteObject(banner_b);
  }

  // Lantern warm amber glow points
  for (int i = 0; i < 4; ++i) {
    int lx = r.left + 14 + i * 65;
    int ly = r.top + h * 46 / 100;
    auto glow_b = CreateSolidBrush(RGB(245, 195, 95));
    auto old_b = SelectObject(dc, glow_b);
    Ellipse(dc, lx - 3, ly - 3, lx + 3, ly + 3);
    SelectObject(dc, old_b);
    DeleteObject(glow_b);
  }

  SelectObject(dc, old_p);
  DrawBorder(dc, r, RGB(55, 80, 115));
}

// Left Decorative Card 3: Floating Azure Crystal Shrine Monument
void DrawCard3(HDC dc, RECT r) {
  int w = r.right - r.left, h = r.bottom - r.top;
  for (int y = 0; y < h; ++y) {
    int g = 14 + y * 20 / h;
    int b = 28 + y * 35 / h;
    Fill(dc, {r.left, r.top + y, r.right, r.top + y + 1}, RGB(10, g, b));
  }
  auto old_p = SelectObject(dc, GetStockObject(NULL_PEN));

  // Circular stone archway
  int cx = r.left + w / 2, cy = r.top + h * 48 / 100, ar = 38;
  auto stone_b = CreateSolidBrush(RGB(22, 32, 50));
  auto old_b = SelectObject(dc, stone_b);
  Ellipse(dc, cx - ar, cy - ar, cx + ar, cy + ar);
  SelectObject(dc, old_b);
  DeleteObject(stone_b);

  auto inner_b = CreateSolidBrush(RGB(12, 18, 30));
  old_b = SelectObject(dc, inner_b);
  Ellipse(dc, cx - ar + 8, cy - ar + 8, cx + ar - 8, cy + ar - 8);
  SelectObject(dc, old_b);
  DeleteObject(inner_b);

  // Soft cyan glow aura
  auto aura_b = CreateSolidBrush(RGB(25, 75, 115));
  old_b = SelectObject(dc, aura_b);
  Ellipse(dc, cx - 22, cy - 22, cx + 22, cy + 22);
  SelectObject(dc, old_b);
  DeleteObject(aura_b);

  // Floating magic diamond crystal
  POINT crystal[] = {
    {cx, cy - 18},
    {cx + 10, cy},
    {cx, cy + 18},
    {cx - 10, cy}
  };
  auto crystal_b = CreateSolidBrush(RGB(85, 215, 255));
  old_b = SelectObject(dc, crystal_b);
  Polygon(dc, crystal, 4);
  SelectObject(dc, old_b);
  DeleteObject(crystal_b);

  // Highlight facet
  POINT facet[] = {
    {cx, cy - 18},
    {cx + 5, cy},
    {cx, cy + 18}
  };
  auto facet_b = CreateSolidBrush(RGB(180, 240, 255));
  old_b = SelectObject(dc, facet_b);
  Polygon(dc, facet, 3);
  SelectObject(dc, old_b);
  DeleteObject(facet_b);

  // Pedestal & basin
  POINT pedestal[] = {
    {cx - 24, cy + 28},
    {cx + 24, cy + 28},
    {cx + 34, r.bottom - 4},
    {cx - 34, r.bottom - 4}
  };
  auto ped_b = CreateSolidBrush(RGB(18, 26, 42));
  old_b = SelectObject(dc, ped_b);
  Polygon(dc, pedestal, 4);
  SelectObject(dc, old_b);
  DeleteObject(ped_b);

  SelectObject(dc, old_p);
  DrawBorder(dc, r, RGB(55, 80, 115));
}

// Step Progress Track (1) Disc 1 — (2) Disc 2 — (3) DLC
void DrawStepTrack(HDC dc, HFONT font, int cx, int y, int current_step, bool spanish) {
  int x1 = cx - 180, x2 = cx, x3 = cx + 180;
  COLORREF cyan_glow = RGB(70, 185, 245);
  COLORREF dark_cyan = RGB(22, 65, 105);
  COLORREF muted_line = RGB(45, 65, 95);
  COLORREF muted_circ = RGB(25, 38, 58);
  COLORREF muted_text = RGB(140, 165, 195);
  COLORREF bright_text = RGB(240, 245, 255);

  // Connecting track line
  auto line_pen = CreatePen(PS_SOLID, 2, muted_line);
  auto old_p = SelectObject(dc, line_pen);
  MoveToEx(dc, x1 + 18, y, nullptr); LineTo(dc, x2 - 18, y);
  MoveToEx(dc, x2 + 18, y, nullptr); LineTo(dc, x3 - 18, y);
  SelectObject(dc, old_p);
  DeleteObject(line_pen);

  if (current_step >= 2) {
    auto active_pen = CreatePen(PS_SOLID, 2, cyan_glow);
    old_p = SelectObject(dc, active_pen);
    MoveToEx(dc, x1 + 18, y, nullptr); LineTo(dc, x2 - 18, y);
    if (current_step >= 3) { MoveToEx(dc, x2 + 18, y, nullptr); LineTo(dc, x3 - 18, y); }
    SelectObject(dc, old_p);
    DeleteObject(active_pen);
  }

  const int coords[] = {x1, x2, x3};
  const wchar_t* nums[] = {L"1", L"2", L"3"};
  const wchar_t* labels_en[] = {L"Disc 1", L"Disc 2", L"DLC (Optional)"};
  const wchar_t* labels_es[] = {L"Disc 1", L"Disc 2", L"DLC (Opcional)"};

  for (int i = 0; i < 3; ++i) {
    int px = coords[i];
    bool active = (i + 1 <= current_step);
    // Outer circle
    auto pen = CreatePen(PS_SOLID, 2, active ? cyan_glow : muted_line);
    auto brush = CreateSolidBrush(active ? dark_cyan : muted_circ);
    auto prev_pen = SelectObject(dc, pen);
    auto prev_brush = SelectObject(dc, brush);
    Ellipse(dc, px - 14, y - 14, px + 14, y + 14);
    SelectObject(dc, prev_brush);
    SelectObject(dc, prev_pen);
    DeleteObject(brush);
    DeleteObject(pen);

    // Number
    Label(dc, font, {px - 14, y - 9, px + 14, y + 14}, nums[i], active ? RGB(255, 255, 255) : muted_text, DT_CENTER | DT_SINGLELINE);

    // Label below
    const wchar_t* lbl = spanish ? labels_es[i] : labels_en[i];
    Label(dc, font, {px - 65, y + 18, px + 65, y + 40}, lbl, active ? bright_text : muted_text, DT_CENTER | DT_SINGLELINE);
  }
}

// Status checkmark / info badge
void DrawStatusBadge(HDC dc, HFONT bold_font, HFONT small_font, int x, int y, int type,
                     const std::wstring& line1, const std::wstring& line2) {
  if (line1.empty()) return;

  // Icon circle
  COLORREF icon_bg = (type == 1) ? RGB(45, 175, 115) : (type == 2 ? RGB(40, 125, 205) : RGB(195, 60, 60));
  COLORREF text_color = (type == 1) ? RGB(72, 215, 145) : (type == 2 ? RGB(90, 180, 240) : RGB(240, 95, 95));
  const wchar_t* icon_char = (type == 1) ? L"✓" : (type == 2 ? L"ℹ" : L"✕");

  if (type != 0) {
    auto b = CreateSolidBrush(icon_bg);
    auto old_b = SelectObject(dc, b);
    auto old_p = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, x, y + 1, x + 18, y + 19);
    SelectObject(dc, old_p);
    SelectObject(dc, old_b);
    DeleteObject(b);

    Label(dc, small_font, {x, y + 2, x + 18, y + 18}, icon_char, RGB(255, 255, 255), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }

  int text_x = (type != 0) ? x + 24 : x;
  Label(dc, bold_font, {text_x, y, text_x + 500, y + 18}, line1, (type != 0 ? text_color : RGB(140, 160, 185)), DT_LEFT | DT_SINGLELINE);
  if (!line2.empty()) {
    Label(dc, small_font, {text_x, y + 18, text_x + 500, y + 34}, line2, RGB(145, 180, 165), DT_LEFT | DT_SINGLELINE);
  }
}

// Main interactive Wizard dialog state
struct WizardView {
  fs::path exe_dir;

  std::optional<iu::assets::Disc> first;
  std::optional<iu::assets::Disc> second;
  std::vector<iu::dlc::Package> dlc;

  std::wstring disc1_path_str;
  std::wstring disc2_path_str;
  std::wstring dlc_path_str;

  std::wstring disc1_line1;
  std::wstring disc1_line2;
  int disc1_badge = 0;

  std::wstring disc2_line1;
  std::wstring disc2_line2;
  int disc2_badge = 0;

  std::wstring dlc_line1;
  std::wstring dlc_line2;
  int dlc_badge = 2; // info by default

  bool installing = false;
  std::atomic<bool> cancel_install{false};
  std::atomic<unsigned> install_percent{0};
  std::wstring install_status;
  std::future<fs::path> install_future;

  HWND hwnd = nullptr;
  HWND edit1 = nullptr, btn1 = nullptr;
  HWND edit2 = nullptr, btn2 = nullptr;
  HWND edit3 = nullptr, btn3 = nullptr;
  HWND btn_back = nullptr, btn_next = nullptr, btn_cancel = nullptr;
  HWND btn_en = nullptr, btn_es = nullptr;

  HFONT font_title = nullptr;
  HFONT font_sub = nullptr;
  HFONT font_head = nullptr;
  HFONT font_body = nullptr;
  HFONT font_bold = nullptr;
  HFONT font_btn = nullptr;

  HBRUSH brush_input = nullptr;
  std::optional<fs::path> result_root;
  bool done = false;

  void UpdateTexts() {
    SetWindowTextW(btn_en, L"English");
    SetWindowTextW(btn_es, L"Español");
    SetWindowTextW(btn1, Text(L"Examinar...").c_str());
    SetWindowTextW(btn2, Text(L"Examinar...").c_str());
    SetWindowTextW(btn3, Text(L"Examinar...").c_str());
    SetWindowTextW(btn_back, Text(L"Atras").c_str());
    SetWindowTextW(btn_next, Text(L"Siguiente >").c_str());
    SetWindowTextW(btn_cancel, Text(L"Cancelar").c_str());

    if (!first) {
      disc1_line1 = Text(L"Seleccione la ISO o carpeta extraida del Disc 1.");
      disc1_line2.clear();
      disc1_badge = 0;
    }
    if (!second) {
      disc2_line1 = Text(L"Seleccione la ISO o carpeta extraida del Disc 2.");
      disc2_line2.clear();
      disc2_badge = 0;
    }
    if (dlc.empty()) {
      dlc_line1 = Text(L"Sin DLC seleccionado. Puede omitir este paso y anadir DLC mas tarde.");
      dlc_line2.clear();
      dlc_badge = 2;
    }
    EnableWindow(btn_next, first.has_value());
    InvalidateRect(hwnd, nullptr, FALSE);
  }

  void PickDisc(int slot) {
    int choice = Dialog(L"Tipo de fuente", L"La identidad se lee del XEX, nunca del nombre de la carpeta.",
                        {{201, L"Imagen ISO"}, {202, L"Carpeta extraida"}});
    if (choice == IDCANCEL) return;
    auto picked = iu::assets::Pick(choice == 202, hwnd);
    if (!picked) return;

    try {
      auto d = iu::assets::Inspect(*picked);
      if (d.number != unsigned(slot)) {
        throw std::runtime_error(Text("El disco seleccionado no corresponde a este paso."));
      }
      if (slot == 1) {
        if (second) iu::assets::ValidatePair(d, second);
        first = std::move(d);
        disc1_path_str = first->source.wstring();
        disc1_line1 = Text(L"Detectado: Infinite Undiscovery — ") + Wide(first->edition == "USA" ? "NTSC-U" : first->edition) + L" — Disc 1";
        disc1_line2 = Text(L"Archivos del juego verificados.");
        disc1_badge = 1;
        SetWindowTextW(edit1, disc1_path_str.c_str());
      } else {
        if (first) iu::assets::ValidatePair(*first, d);
        second = std::move(d);
        disc2_path_str = second->source.wstring();
        disc2_line1 = Text(L"Detectado: Infinite Undiscovery — ") + Wide(second->edition == "USA" ? "NTSC-U" : second->edition) + L" — Disc 2";
        disc2_line2 = Text(L"Archivos del juego verificados.");
        disc2_badge = 1;
        SetWindowTextW(edit2, disc2_path_str.c_str());
      }
    } catch (const std::exception& e) {
      if (slot == 1) {
        disc1_badge = 3;
        disc1_line1 = Wide(e.what());
        disc1_line2.clear();
        first.reset();
      } else {
        disc2_badge = 3;
        disc2_line1 = Wide(e.what());
        disc2_line2.clear();
        second.reset();
      }
    }
    UpdateTexts();
  }

  void PickDlcAction() {
    auto choice = Dialog(L"Contenido descargable opcional",
                         L"Seleccione los paquetes STFS ya extraidos. No se necesita RAR ni UnRAR. A Voucher y B Voucher se mantendran como contenidos independientes. Puede continuar sin DLC.",
                         {{211, L"Elegir paquetes STFS"}, {212, L"Elegir carpeta con paquetes STFS"}, {213, L"Continuar sin DLC"}});
    if (choice == 213) {
      dlc.clear();
      dlc_path_str.clear();
      SetWindowTextW(edit3, L"");
      dlc_badge = 2;
      dlc_line1 = Text(L"Sin DLC seleccionado. Puede omitir este paso y anadir DLC mas tarde.");
      dlc_line2.clear();
      InvalidateRect(hwnd, nullptr, FALSE);
      return;
    }
    std::vector<fs::path> sources;
    if (choice == 211) sources = iu::assets::PickPackages(hwnd);
    if (choice == 212) {
      if (auto folder = iu::assets::Pick(true, hwnd)) {
        for (const auto& e : fs::directory_iterator(*folder))
          if (e.is_regular_file()) sources.push_back(e.path());
      }
    }
    if (sources.empty()) return;

    try {
      std::vector<iu::dlc::Package> list;
      for (const auto& s : sources) list.push_back(iu::dlc::Inspect(s));
      iu::dlc::ValidateSelection(list);
      dlc = std::move(list);
      dlc_badge = 1;
      dlc_path_str = std::to_wstring(dlc.size()) + Text(L" paquete(s) DLC");
      SetWindowTextW(edit3, dlc_path_str.c_str());
      dlc_line1 = Text(L"DLC detectado: ") + std::to_wstring(dlc.size()) + Text(L" paquetes DLC verificados.");
      dlc_line2.clear();
    } catch (const std::exception& e) {
      dlc.clear();
      dlc_badge = 3;
      dlc_line1 = Wide(e.what());
      dlc_line2.clear();
    }
    UpdateTexts();
  }

  void StartInstall() {
    if (!first) return;
    installing = true;
    cancel_install = false;
    install_percent = 0;
    install_status = Text(L"Copiando y verificando archivos del juego...");

    // Hide input rows
    ShowWindow(edit1, SW_HIDE); ShowWindow(btn1, SW_HIDE);
    ShowWindow(edit2, SW_HIDE); ShowWindow(btn2, SW_HIDE);
    ShowWindow(edit3, SW_HIDE); ShowWindow(btn3, SW_HIDE);
    EnableWindow(btn_next, FALSE);

    install_future = std::async(std::launch::async, [&]() {
      return iu::assets::Install(*first, second, exe_dir,
        [&](uint64_t n, uint64_t total) {
          if (total > 0) install_percent = static_cast<unsigned>(100 * n / total);
        },
        [&]() { return cancel_install.load(); },
        dlc);
    });
    SetTimer(hwnd, 201, 50, nullptr);
    InvalidateRect(hwnd, nullptr, FALSE);
  }

  void CancelAction() {
    if (installing) {
      int confirm = Dialog(L"¿Cancelar la instalacion?",
                           L"Los archivos originales se conservan. Solo se limpiaran los temporales.",
                           {{301, L"Seguir instalando"}, {302, L"Cancelar instalacion"}});
      if (confirm != 302) return;
      cancel_install = true;
      try { install_future.get(); } catch (...) {}
      installing = false;
      KillTimer(hwnd, 201);
      // Restore rows
      ShowWindow(edit1, SW_SHOW); ShowWindow(btn1, SW_SHOW);
      ShowWindow(edit2, SW_SHOW); ShowWindow(btn2, SW_SHOW);
      ShowWindow(edit3, SW_SHOW); ShowWindow(btn3, SW_SHOW);
      EnableWindow(btn_next, first.has_value());
      InvalidateRect(hwnd, nullptr, FALSE);
      return;
    }
    done = true;
    DestroyWindow(hwnd);
  }
};

void PaintWizard(WizardView& v, HDC dc) {
  RECT rc; GetClientRect(v.hwnd, &rc);

  // Background deep midnight gradient
  int h = rc.bottom - rc.top;
  for (int y = 0; y < h; ++y) {
    int g = 15 + y * 10 / h;
    int b = 26 + y * 16 / h;
    Fill(dc, {rc.left, rc.top + y, rc.right, rc.top + y + 1}, RGB(10, g, b));
  }
  // Subtle starry sky dust
  for (int i = 0; i < 40; ++i) {
    int sx = (i * 73 + 17) % (rc.right - 40) + 20;
    int sy = (i * 37 + 11) % 120 + 10;
    int b = 180 + (i * 23) % 75;
    SetPixel(dc, sx, sy, RGB(b - 30, b - 15, b));
  }

  // Top header: Crest and title
  DrawCrest(dc, 52, 48, 22);
  Label(dc, v.font_title, {86, 20, 600, 56}, L"Infinite Undiscovery", RGB(240, 245, 255));
  Label(dc, v.font_sub, {88, 56, 600, 78}, L"R E C O M P", RGB(175, 195, 225));
  Label(dc, v.font_head, {87, 78, 600, 108}, Text(L"Configuracion de archivos"), RGB(215, 230, 250));

  // Language label
  Label(dc, v.font_body, {725, 30, 800, 54}, Text(L"Idioma") + L":", RGB(150, 175, 205), DT_RIGHT | DT_SINGLELINE);

  // Left decorative 3 cards
  DrawCard1(dc, {38, 135, 262, 275});
  DrawCard2(dc, {38, 290, 262, 430});
  DrawCard3(dc, {38, 445, 262, 585});

  // Main Card
  RECT card_rc = {285, 135, 1000, 630};
  DrawRoundRect(dc, card_rc, 12, RGB(45, 70, 105), RGB(14, 22, 36));

  // Step Progress Track
  int current_step = v.installing ? 3 : (v.first ? (v.second ? 2 : 1) : 1);
  DrawStepTrack(dc, v.font_body, 642, 175, current_step, es);

  if (!v.installing) {
    // Headline
    Label(dc, v.font_bold, {315, 224, 980, 248}, L"✦  " + Text(L"Listo para configurar los archivos del juego."), RGB(240, 245, 255));
    Label(dc, v.font_body, {337, 248, 980, 272}, Text(L"Seleccione los discos del juego y DLC opcional. El asistente detectara y verificara sus archivos."), RGB(150, 175, 205));

    // Disc Icons
    DrawDiscIcon(dc, 335, 290, 17);
    DrawDiscIcon(dc, 335, 365, 17);
    DrawDiscIcon(dc, 335, 440, 17);

    // Row Labels
    Label(dc, v.font_bold, {360, 280, 475, 304}, Text(L"Disc 1 (Requerido)"), RGB(230, 240, 255));
    Label(dc, v.font_bold, {360, 355, 475, 379}, Text(L"Disc 2 (Recomendado)"), RGB(230, 240, 255));
    Label(dc, v.font_bold, {360, 430, 475, 454}, Text(L"DLC (Opcional)"), RGB(230, 240, 255));

    // Status Badges
    DrawStatusBadge(dc, v.font_bold, v.font_body, 360, 308, v.disc1_badge, v.disc1_line1, v.disc1_line2);
    DrawStatusBadge(dc, v.font_bold, v.font_body, 360, 383, v.disc2_badge, v.disc2_line1, v.disc2_line2);
    DrawStatusBadge(dc, v.font_bold, v.font_body, 360, 458, v.dlc_badge, v.dlc_line1, v.dlc_line2);
  } else {
    // Installation view
    Label(dc, v.font_head, {320, 240, 970, 275}, v.install_status, RGB(240, 245, 255), DT_CENTER);

    // Progress Bar
    RECT bar_rc = {360, 340, 925, 368};
    DrawRoundRect(dc, bar_rc, 6, RGB(45, 70, 105), RGB(10, 16, 26));

    unsigned pct = std::min(100u, v.install_percent.load());
    if (pct > 0) {
      int fill_w = int((bar_rc.right - bar_rc.left - 4) * pct / 100);
      RECT fill_rc = {bar_rc.left + 2, bar_rc.top + 2, bar_rc.left + 2 + fill_w, bar_rc.bottom - 2};
      auto fill_b = CreateSolidBrush(RGB(50, 165, 235));
      FillRect(dc, &fill_rc, fill_b);
      DeleteObject(fill_b);
    }
    std::wstring pct_str = std::to_wstring(pct) + L"%";
    Label(dc, v.font_bold, {360, 378, 925, 405}, pct_str, RGB(220, 235, 255), DT_CENTER);
  }
}

LRESULT CALLBACK WizardProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
  auto* v = reinterpret_cast<WizardView*>(GetWindowLongPtrW(w, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    v = static_cast<WizardView*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
    v->hwnd = w;
    SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(v));
  }
  if (!v) return DefWindowProcW(w, msg, wp, lp);

  switch (msg) {
    case WM_ERASEBKGND: return 1;

    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(w, &ps);
      HDC back = CreateCompatibleDC(dc);
      RECT r; GetClientRect(w, &r);
      auto bitmap = CreateCompatibleBitmap(dc, r.right, r.bottom);
      auto prev = SelectObject(back, bitmap);
      PaintWizard(*v, back);
      BitBlt(dc, 0, 0, r.right, r.bottom, back, 0, 0, SRCCOPY);
      SelectObject(back, prev);
      DeleteObject(bitmap);
      DeleteDC(back);
      EndPaint(w, &ps);
      return 0;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
      HDC dc = reinterpret_cast<HDC>(wp);
      SetTextColor(dc, RGB(200, 220, 245));
      SetBkColor(dc, RGB(11, 16, 26));
      return reinterpret_cast<LRESULT>(v->brush_input);
    }

    case WM_DRAWITEM: {
      auto* d = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
      bool is_lang = (d->CtlID == 901 || d->CtlID == 902);
      bool is_primary = (d->CtlID == 1011);
      bool is_browse = (d->CtlID == 1002 || d->CtlID == 1004 || d->CtlID == 1006);

      COLORREF bg = RGB(18, 28, 44);
      COLORREF border = RGB(45, 68, 98);
      COLORREF text = RGB(200, 215, 235);

      if (is_lang) {
        bool active = (d->CtlID == (es ? 902 : 901));
        bg = active ? RGB(26, 68, 120) : RGB(14, 22, 36);
        border = active ? RGB(70, 150, 235) : RGB(35, 52, 78);
        text = active ? RGB(255, 255, 255) : RGB(150, 175, 205);
      } else if (is_primary) {
        bool enabled = IsWindowEnabled(d->hwndItem);
        bg = enabled ? RGB(22, 75, 138) : RGB(16, 24, 38);
        border = enabled ? RGB(65, 160, 245) : RGB(32, 48, 70);
        text = enabled ? RGB(255, 255, 255) : RGB(80, 105, 135);
      } else if (is_browse) {
        bg = RGB(20, 32, 50);
        border = RGB(48, 75, 110);
        text = RGB(220, 235, 250);
      }

      DrawRoundRect(d->hDC, d->rcItem, is_lang ? 14 : 6, border, bg);

      wchar_t label[160]{};
      GetWindowTextW(d->hwndItem, label, 160);
      Label(d->hDC, (is_primary || is_lang) ? v->font_bold : v->font_btn, d->rcItem, label, text,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

      if (d->itemState & ODS_FOCUS) DrawFocusRect(d->hDC, &d->rcItem);
      return TRUE;
    }

    case WM_COMMAND: {
      int id = LOWORD(wp);
      if (id == 901) { // English
        SetSpanish(false); SaveLanguage(); v->UpdateTexts();
      } else if (id == 902) { // Español
        SetSpanish(true); SaveLanguage(); v->UpdateTexts();
      } else if (id == 1002) { // Browse Disc 1
        v->PickDisc(1);
      } else if (id == 1004) { // Browse Disc 2
        v->PickDisc(2);
      } else if (id == 1006) { // Browse DLC
        v->PickDlcAction();
      } else if (id == 1011) { // Next / Install
        v->StartInstall();
      } else if (id == IDCANCEL || id == 2) {
        v->CancelAction();
      }
      return 0;
    }

    case WM_TIMER: {
      if (wp == 201 && v->installing) {
        InvalidateRect(w, nullptr, FALSE);
        if (v->install_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          KillTimer(w, 201);
          try {
            v->result_root = v->install_future.get();
            SaveLanguage();
            v->done = true;
            DestroyWindow(w);
          } catch (const std::exception& e) {
            v->installing = false;
            // Restore controls
            ShowWindow(v->edit1, SW_SHOW); ShowWindow(v->btn1, SW_SHOW);
            ShowWindow(v->edit2, SW_SHOW); ShowWindow(v->btn2, SW_SHOW);
            ShowWindow(v->edit3, SW_SHOW); ShowWindow(v->btn3, SW_SHOW);
            EnableWindow(v->btn_next, v->first.has_value());
            Error(e.what());
          }
        }
      }
      return 0;
    }

    case WM_USER + 102: // TDM_CLICK_BUTTON
      v->CancelAction();
      return 0;

    case WM_CLOSE:
      v->CancelAction();
      return 0;
  }
  return DefWindowProcW(w, msg, wp, lp);
}

// Dialog window view for modal confirmations and error dialogs
struct ModalDialogView {
  std::wstring heading, body;
  std::vector<Button> buttons;
  std::function<bool(int)> action;
  std::function<std::wstring()> poll;
  HWND hwnd{}, language{};
  HFONT font_head{}, font_body{}, font_brand{};
  int result = IDCANCEL;
  bool done = false;
  std::wstring progress;

  void Refresh() {
    for (auto& b : buttons) SetWindowTextW(GetDlgItem(hwnd, b.id), Text(b.label).c_str());
    SetWindowTextW(GetDlgItem(hwnd, IDCANCEL), Text(L"Cancelar").c_str());
    InvalidateRect(hwnd, nullptr, FALSE);
  }
  void Choose(int id) {
    if (action && !action(id)) return;
    result = id; done = true; DestroyWindow(hwnd);
  }
};

void PaintModalDialog(ModalDialogView& v, HDC dc) {
  RECT area; GetClientRect(v.hwnd, &area);
  Fill(dc, area, RGB(12, 18, 30));

  // Left banner gradient and crest
  Fill(dc, {0, 0, 260, area.bottom}, RGB(16, 25, 42));
  DrawCard3(dc, {20, 25, 240, 220});
  DrawCrest(dc, 130, 290, 24);
  Label(dc, v.font_brand, {15, 335, 245, 420}, L"INFINITE\nUNDISCOVERY", RGB(240, 245, 255), DT_CENTER);
  Label(dc, v.font_body, {15, 430, 245, 470}, L"R E C O M P", RGB(175, 195, 225), DT_CENTER);

  // Main card
  RECT card = {280, 25, area.right - 25, area.bottom - 25};
  DrawRoundRect(dc, card, 10, RGB(45, 70, 105), RGB(16, 24, 38));

  Label(dc, v.font_head, {310, 45, area.right - 45, 95}, Text(v.heading), RGB(240, 245, 255));
  auto body = Text(v.body);
  bool verified = body.find(L"Verified") != std::wstring::npos || body.find(L"validado") != std::wstring::npos;
  Fill(dc, {310, 105, 314, 320}, verified ? RGB(72, 215, 145) : RGB(70, 150, 230));
  Label(dc, v.font_body, {330, 105, area.right - 50, 320}, body, verified ? RGB(160, 225, 195) : RGB(205, 220, 240));

  if (!v.progress.empty()) {
    Label(dc, v.font_head, {330, 330, area.right - 50, 375}, v.progress, RGB(225, 235, 255));
  }
}

LRESULT CALLBACK ModalProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
  auto* v = reinterpret_cast<ModalDialogView*>(GetWindowLongPtrW(w, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    v = static_cast<ModalDialogView*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
    v->hwnd = w;
    SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(v));
  }
  if (!v) return DefWindowProcW(w, msg, wp, lp);

  switch (msg) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
      PAINTSTRUCT ps; HDC dc = BeginPaint(w, &ps);
      HDC back = CreateCompatibleDC(dc);
      RECT r; GetClientRect(w, &r);
      auto bmp = CreateCompatibleBitmap(dc, r.right, r.bottom);
      auto prev = SelectObject(back, bmp);
      PaintModalDialog(*v, back);
      BitBlt(dc, 0, 0, r.right, r.bottom, back, 0, 0, SRCCOPY);
      SelectObject(back, prev);
      DeleteObject(bmp); DeleteDC(back);
      EndPaint(w, &ps);
      return 0;
    }
    case WM_DRAWITEM: {
      auto* d = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
      bool primary = (d->CtlID == 105 || d->CtlID == 207 || d->CtlID == 301 || d->CtlID >= 400);
      COLORREF bg = primary ? RGB(22, 75, 138) : RGB(18, 28, 44);
      COLORREF border = primary ? RGB(65, 160, 245) : RGB(45, 68, 98);
      DrawRoundRect(d->hDC, d->rcItem, 6, border, bg);

      wchar_t label[160]{}; GetWindowTextW(d->hwndItem, label, 160);
      Label(d->hDC, v->font_body, d->rcItem, label, RGB(240, 245, 255), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      if (d->itemState & ODS_FOCUS) DrawFocusRect(d->hDC, &d->rcItem);
      return TRUE;
    }
    case WM_COMMAND:
      if (LOWORD(wp) == 900 && HIWORD(wp) == CBN_SELCHANGE) {
        SetSpanish(SendMessageW(v->language, CB_GETCURSEL, 0, 0) == 1);
        SaveLanguage(); v->Refresh(); return 0;
      } else if (HIWORD(wp) == BN_CLICKED) {
        v->Choose(LOWORD(wp)); return 0;
      }
      break;
    case WM_TIMER:
      if (v->poll) {
        v->progress = v->poll(); InvalidateRect(w, nullptr, FALSE);
        if (v->action && v->action(IDOK)) { v->result = IDOK; v->done = true; DestroyWindow(w); }
      }
      return 0;
    case WM_USER + 102: // TDM_CLICK_BUTTON
      v->Choose(static_cast<int>(wp));
      return 0;
    case WM_CLOSE: v->Choose(IDCANCEL); return 0;
  }
  return DefWindowProcW(w, msg, wp, lp);
}

} // namespace

std::wstring Wide(const std::string& s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
  std::wstring out(n, 0); MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), out.data(), n); return out;
}

bool Spanish() { return es; }
void SetSpanish(bool value) { es = value; }

std::string Text(const std::string& input) {
  auto s = input; std::vector<const std::pair<std::string,std::string>*> ordered;
  for (auto& p : strings) ordered.push_back(&p);
  std::sort(ordered.begin(), ordered.end(), [](auto a, auto b) {
    return std::max(a->first.size(), a->second.size()) > std::max(b->first.size(), b->second.size());
  });
  for (auto p : ordered) Replace(s, es ? p->second : p->first, es ? p->first : p->second);
  return s;
}

std::wstring Text(const std::wstring& s) { return Wide(Text(Utf8(s))); }

Paths Layout(const fs::path& exe, const std::string& region) {
  if (region != "NTSC-U" && region != "PAL") throw std::runtime_error(Text("Region no valida."));
  auto root = fs::absolute(exe) / region; NoLinks(root);
  return {root, root / "assets", root / "saves", root / "shaders", root / "cache", root / "logs", root / "config.json"};
}

void Initialize(const fs::path& exe) {
  home = fs::absolute(exe); es = false;
  NoLinks(home / "setup.json");
  std::ifstream in(home / "setup.json");
  std::string s((std::istreambuf_iterator<char>(in)), {});
  es = s.find("\"es\"") != std::string::npos;
}

void SaveLanguage() {
  if (home.empty()) return;
  WriteConfig(home / "setup.json");
  for (auto region : {"NTSC-U", "PAL"}) {
    auto p = Layout(home, region);
    if (fs::is_directory(p.root)) WriteConfig(p.config);
  }
}

int Dialog(const std::wstring& heading, const std::wstring& body, const std::vector<Button>& buttons,
           std::function<bool(int)> action, std::function<std::wstring()> poll) {
  static bool registered = false;
  if (!registered) {
    WNDCLASSW wc{}; wc.lpfnWndProc = ModalProc; wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"IUModalDialog"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc); registered = true;
  }
  ModalDialogView v; v.heading = heading; v.body = body; v.buttons = buttons; v.action = action; v.poll = poll;
  v.font_body = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  v.font_head = CreateFontW(-24, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  v.font_brand = CreateFontW(-22, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Georgia");

  auto owner = GetActiveWindow();
  if (owner) EnableWindow(owner, FALSE);

  HWND w = CreateWindowExW(WS_EX_DLGMODALFRAME, L"IUModalDialog", L"Infinite Undiscovery - Asset Setup",
                           WS_CAPTION | WS_SYSMENU,
                           (GetSystemMetrics(SM_CXSCREEN) - 860) / 2, (GetSystemMetrics(SM_CYSCREEN) - 520) / 2,
                           860, 520, owner, nullptr, GetModuleHandleW(nullptr), &v);
  if (!w) throw std::runtime_error("Cannot create modal dialog");

  for (size_t i = 0; i < buttons.size(); ++i) {
    auto b = buttons[i];
    auto h = CreateWindowW(L"BUTTON", Text(b.label).c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                           310 + int(i % 2) * 255, 360 + int(i / 2) * 50, 240, 42, w, (HMENU)(INT_PTR)b.id, nullptr, nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)v.font_body, TRUE);
  }
  auto cancel_h = CreateWindowW(L"BUTTON", Text(L"Cancelar").c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                700, 420, 110, 36, w, (HMENU)IDCANCEL, nullptr, nullptr);
  SendMessageW(cancel_h, WM_SETFONT, (WPARAM)v.font_body, TRUE);

  if (poll) SetTimer(w, 1, 100, nullptr);
  ShowWindow(w, SW_SHOW); UpdateWindow(w);
  MSG m;
  while (!v.done && GetMessageW(&m, nullptr, 0, 0) > 0) {
    if (!IsDialogMessageW(w, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
  }
  if (owner) { EnableWindow(owner, TRUE); SetActiveWindow(owner); }
  DeleteObject(v.font_body); DeleteObject(v.font_head); DeleteObject(v.font_brand);
  return v.result;
}

void Error(const std::string& message) {
  Dialog(L"Error", Wide(message), {{IDOK, L"Cerrar"}});
}

std::optional<fs::path> RunWizard(const fs::path& exe) {
  auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  struct ComScope { bool ok; ~ComScope() { if (ok) CoUninitialize(); } } com_scope{SUCCEEDED(hr)};
  INITCOMMONCONTROLSEX icx{sizeof(icx), ICC_STANDARD_CLASSES}; InitCommonControlsEx(&icx);

  static bool registered = false;
  if (!registered) {
    WNDCLASSW wc{}; wc.lpfnWndProc = WizardProc; wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"IUPortableWizard"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc); registered = true;
  }

  WizardView v;
  v.exe_dir = exe;

  v.font_title = CreateFontW(-28, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Georgia");
  v.font_sub = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  v.font_head = CreateFontW(-20, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  v.font_body = CreateFontW(-14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  v.font_bold = CreateFontW(-14, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  v.font_btn = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  v.brush_input = CreateSolidBrush(RGB(11, 16, 26));

  auto owner = GetActiveWindow();
  if (owner) EnableWindow(owner, FALSE);

  int win_w = 1040, win_h = 680;
  HWND w = CreateWindowExW(WS_EX_DLGMODALFRAME, L"IUPortableWizard", L"Infinite Undiscovery - Asset Setup",
                           WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           (GetSystemMetrics(SM_CXSCREEN) - win_w) / 2, (GetSystemMetrics(SM_CYSCREEN) - win_h) / 2,
                           win_w, win_h, owner, nullptr, GetModuleHandleW(nullptr), &v);
  if (!w) throw std::runtime_error("Cannot create wizard window");

  // Language buttons
  v.btn_en = CreateWindowW(L"BUTTON", L"English", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                           805, 26, 85, 26, w, (HMENU)901, nullptr, nullptr);
  v.btn_es = CreateWindowW(L"BUTTON", L"Español", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                           895, 26, 85, 26, w, (HMENU)902, nullptr, nullptr);

  // Edit boxes and browse buttons
  v.edit1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                            480, 275, 375, 26, w, (HMENU)1001, nullptr, nullptr);
  v.btn1 = CreateWindowW(L"BUTTON", Text(L"Examinar...").c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                         865, 275, 110, 26, w, (HMENU)1002, nullptr, nullptr);

  v.edit2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                            480, 350, 375, 26, w, (HMENU)1003, nullptr, nullptr);
  v.btn2 = CreateWindowW(L"BUTTON", Text(L"Examinar...").c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                         865, 350, 110, 26, w, (HMENU)1004, nullptr, nullptr);

  v.edit3 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                            480, 425, 375, 26, w, (HMENU)1005, nullptr, nullptr);
  v.btn3 = CreateWindowW(L"BUTTON", Text(L"Examinar...").c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                         865, 425, 110, 26, w, (HMENU)1006, nullptr, nullptr);

  SendMessageW(v.edit1, WM_SETFONT, (WPARAM)v.font_body, TRUE);
  SendMessageW(v.edit2, WM_SETFONT, (WPARAM)v.font_body, TRUE);
  SendMessageW(v.edit3, WM_SETFONT, (WPARAM)v.font_body, TRUE);

  // Bottom action buttons
  v.btn_back = CreateWindowW(L"BUTTON", Text(L"Atras").c_str(), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                             670, 570, 75, 34, w, (HMENU)1010, nullptr, nullptr);
  EnableWindow(v.btn_back, FALSE);

  v.btn_next = CreateWindowW(L"BUTTON", Text(L"Siguiente >").c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                             755, 570, 125, 34, w, (HMENU)1011, nullptr, nullptr);
  EnableWindow(v.btn_next, FALSE);

  v.btn_cancel = CreateWindowW(L"BUTTON", Text(L"Cancelar").c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                               890, 570, 85, 34, w, (HMENU)IDCANCEL, nullptr, nullptr);

  v.UpdateTexts();
  ShowWindow(w, SW_SHOW);
  UpdateWindow(w);

  MSG m;
  while (!v.done && GetMessageW(&m, nullptr, 0, 0) > 0) {
    if (!IsDialogMessageW(w, &m)) {
      TranslateMessage(&m);
      DispatchMessageW(&m);
    }
  }

  if (owner) { EnableWindow(owner, TRUE); SetActiveWindow(owner); }

  DeleteObject(v.font_title);
  DeleteObject(v.font_sub);
  DeleteObject(v.font_head);
  DeleteObject(v.font_body);
  DeleteObject(v.font_bold);
  DeleteObject(v.font_btn);
  DeleteObject(v.brush_input);

  return v.result_root;
}

std::optional<fs::path> Select(const fs::path& exe, bool maintenance) {
  std::vector<Paths> installed;
  for (auto region : {"NTSC-U", "PAL"}) {
    auto p = Layout(exe, region);
    if (assets::Ready(p.assets / "disc1")) {
      auto d = assets::Inspect(p.assets / "disc1");
      if ((d.edition == "USA" ? "NTSC-U" : "PAL") != std::string(region))
        throw std::runtime_error(Text("Ruta portable invalida."));
      installed.push_back(p);
    }
  }

  std::optional<fs::path> root;
  if (installed.size() == 1 && !maintenance) {
    root = installed.front().root;
  } else if (!installed.empty()) {
    std::vector<Button> choices;
    for (size_t i = 0; i < installed.size(); ++i) {
      choices.push_back({400 + int(i), L"✓ " + installed[i].root.filename().wstring()});
    }
    choices.push_back({499, L"Configurar otros discos"});
    int id = Dialog(L"Instalaciones verificadas", L"Elija una instalacion detectada o configure otros discos.", choices);
    if (id == IDCANCEL) return {};
    if (id >= 400 && id < 400 + int(installed.size())) root = installed[id - 400].root;
  }

  if (!root) {
auto path = RunWizard(exe);
    if (!path) return {};
    root = path->parent_path().parent_path();
  }

  auto paths = Layout(exe, root->filename().string());
  for (auto path : {paths.saves, paths.shaders, paths.cache, paths.logs, paths.cache / "metadata"}) {
    NoLinks(path);
    fs::create_directories(path);
  }
  SaveLanguage();
  return root;
}

} // namespace iu::portable
