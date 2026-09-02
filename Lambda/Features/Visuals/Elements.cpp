#include "Elements.h"

#include <format>

#include "../../ImGui/imgui_settings.h"
#include "../../ImGui/imgui_internal.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"

extern const char* keys[];

CElements* Elements = new CElements;

static constexpr float PANEL_WIDTH = 208.f;
static constexpr float HEADER_HEIGHT = 25.f;
static constexpr float ROW_HEIGHT = 19.f;
static constexpr float PADDING = 10.f;
static constexpr float BAR_HEIGHT = 4.f;
static constexpr float SETTINGS_ROW = 20.f;
static constexpr float EYE_SPACE = 18.f;
static constexpr float COG_SPACE = 17.f;
static constexpr float LOG_ROW_HEIGHT = 16.f;
static constexpr float LOG_FADE = 0.6f;
static constexpr size_t LOG_LIMIT = 10;

static const ImVec4 col_title = ImVec4(1.f, 1.f, 1.f, 0.9f);
static const ImVec4 col_value = ImVec4(1.f, 1.f, 1.f, 0.7f);
static const ImVec4 col_muted = ImVec4(1.f, 1.f, 1.f, 0.28f);
static const ImVec4 col_good = ImVec4(0.45f, 0.78f, 0.42f, 0.9f);
static const ImVec4 col_bad = ImVec4(0.92f, 0.36f, 0.31f, 0.9f);

static ImVec4 Pick(CColorPicker* picker) {
	return ImVec4(picker->value[0], picker->value[1], picker->value[2], picker->value[3]);
}

static ImVec4 ToVec4(const Color& color) {
	return ImVec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
}

static ImVec4 Lerp4(const ImVec4& a, const ImVec4& b, float t) {
	return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

static ImVec2 g_StylePadding = ImVec2(8.f, 8.f);

static float TextWidth(const std::string& text) {
	return font::tab->CalcTextSizeA(font::tab->FontSize, FLT_MAX, 0.f, text.c_str()).x;
}

static std::string Shorten(const std::string& text, float max_width) {
	if (TextWidth(text) <= max_width)
		return text;

	std::string result = text;

	while (result.size() > 1 && TextWidth(result + "..") > max_width)
		result.pop_back();

	return result + "..";
}
static void Fade(float& alpha, bool visible) {
	alpha = ImClamp(alpha + ImGui::GetIO().DeltaTime * 8.f * (visible ? 1.f : -1.f), 0.f, 1.f);
}

static void Approach(float& value, float target, float speed) {
	value += (target - value) * ImMin(ImGui::GetIO().DeltaTime * speed, 1.f);
}

static float Eased(float& value, float target, float speed) {
	Approach(value, target, speed);
	return value;
}

static std::string Lower(std::string text) {
	for (char& c : text)
		if (c >= 'A' && c <= 'Z')
			c += 32;

	return text;
}

static std::string KeyName(int key) {
	if (key <= 0 || key > 0xA5)
		return "";

	const char* name = keys[key];

	if (!name || name[0] == '-')
		return "";

	return Lower(name);
}

static bool Movable() {
	if (!Menu->IsOpened())
		return false;

	ImGuiWindow* menu = ImGui::FindWindowByName("MENU");

	if (!menu)
		return true;

	return !ImGui::IsMouseHoveringRect(menu->Pos, menu->Pos + menu->Size, false);
}

static float StepAnim(const char* label, float target, float speed) {
	ImGuiStorage* storage = ImGui::GetStateStorage();
	const ImGuiID id = ImGui::GetID(label);

	float value = storage->GetFloat(id, target);
	Approach(value, target, speed);
	storage->SetFloat(id, value);

	return value;
}
static void DrawGear(ImDrawList* draw_list, const ImVec2& center, float radius, float spin, ImU32 color) {
	constexpr int teeth = 6;

	for (int i = 0; i < teeth; i++) {
		const float angle = spin + i * (IM_PI * 2.f / teeth);
		const ImVec2 dir = ImVec2(cosf(angle), sinf(angle));

		draw_list->AddLine(center + dir * (radius * 0.5f), center + dir * (radius * 1.02f), color, 2.1f);
	}

	draw_list->AddCircle(center, radius * 0.55f, color, 14, 1.9f);
}

static void DrawEye(ImDrawList* draw_list, const ImVec2& center, float slash, ImU32 color, ImU32 shadow) {
	constexpr float w = 6.3f;
	constexpr float h = 4.1f;

	draw_list->PathClear();
	draw_list->PathLineTo(ImVec2(center.x - w, center.y));
	draw_list->PathBezierQuadraticCurveTo(ImVec2(center.x, center.y - h * 2.f), ImVec2(center.x + w, center.y), 12);
	draw_list->PathBezierQuadraticCurveTo(ImVec2(center.x, center.y + h * 2.f), ImVec2(center.x - w, center.y), 12);
	draw_list->PathStroke(color, ImDrawFlags_Closed, 1.35f);

	draw_list->AddCircleFilled(center, 1.8f, color, 10);

	if (slash <= 0.015f)
		return;

	const ImVec2 from = ImVec2(center.x - w - 1.3f, center.y + h + 1.7f);
	const ImVec2 to = ImVec2(center.x + w + 1.3f, center.y - h - 1.7f);
	const ImVec2 end = from + (to - from) * ImClamp(slash, 0.f, 1.f);

	draw_list->AddLine(from, end, shadow, 3.4f);
	draw_list->AddLine(from, end, color, 1.5f);
}

static void DrawCheck(ImDrawList* draw_list, const ImVec2& center, float progress, ImU32 color) {
	const ImVec2 a = ImVec2(center.x - 3.4f, center.y + 0.2f);
	const ImVec2 b = ImVec2(center.x - 1.1f, center.y + 2.6f);
	const ImVec2 c = ImVec2(center.x + 3.6f, center.y - 2.6f);

	const float first = ImClamp(progress * 2.2f, 0.f, 1.f);
	const float second = ImClamp((progress - 0.45f) * 1.82f, 0.f, 1.f);

	draw_list->AddLine(a, a + (b - a) * first, color, 1.7f);

	if (second > 0.f)
		draw_list->AddLine(b, b + (c - b) * second, color, 1.7f);
}
class CPanel {
	ImDrawList* m_pDrawList = nullptr;
	ImVec2 m_Position;
	float m_flAlpha = 1.f;
	float m_flCursor = 0.f;
	float m_flWidth = PANEL_WIDTH;
	bool m_bOpen = false;

	bool HitBox(const char* label, float x, float y, float w, float h);

public:
	ImU32 GetColor(const ImVec4& color, float scale = 1.f) const {
		return ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, ImClamp(color.w * m_flAlpha * scale, 0.f, 1.f)));
	}

	void PutText(const std::string& text, float x, float y, const ImVec4& color, float scale = 1.f) {
		m_pDrawList->AddText(font::tab, font::tab->FontSize, m_Position + ImVec2(x, y), GetColor(color, scale), text.c_str());
	}

	bool Begin(const char* name, const ImVec2& default_position, const ImVec2& size, float alpha, float opacity, float gear, float open, float clip_height = 0.f);
	void End();

	bool Header(const std::string& title, const std::string& value, const ImVec4& value_color, bool separator, float gear, float spin, float t = 1.f);
	void Row(const std::string& title, const std::string& value, const std::string& extra, const ImVec4& title_color, const ImVec4& value_color, float t = 1.f, float indent = 0.f);
	bool RowEye(int index, bool interactive, const std::string& title, const std::string& value, const std::string& extra, const ImVec4& title_color, const ImVec4& value_color, const ImVec4& eye_color, float t, float gear, float slash, float& hover);
	void Bar(float fraction, const ImVec4& color, float t = 1.f);
	void Separator(float t);
	void Toggle(const char* label, CCheckBox* box, float t);
	void Slider(const char* label, CSliderInt* slider, const char* suffix, float t);
	void ColorRow(const char* label, CColorPicker* picker, float t);

	void Pad(float amount) { m_flCursor += amount; }
	float Cursor() const { return m_flCursor; }
	ImDrawList* DrawList() const { return m_pDrawList; }
	ImVec2 Position() const { return m_Position; }
	float Alpha() const { return m_flAlpha; }
	float Width() const { return m_flWidth; }
};

bool CPanel::HitBox(const char* label, float x, float y, float w, float h) {
	if (w < 1.f || h < 1.f)
		return false;

	ImGui::SetCursorScreenPos(m_Position + ImVec2(x, y));
	ImGui::InvisibleButton(label, ImVec2(w, h));

	return true;
}
bool CPanel::Begin(const char* name, const ImVec2& default_position, const ImVec2& size, float alpha, float opacity, float gear, float open, float clip_height) {
	m_flAlpha = alpha;
	m_flCursor = 0.f;
	m_flWidth = size.x;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	if (!Movable())
		flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;

	g_StylePadding = ImGui::GetStyle().WindowPadding;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

	ImGui::SetNextWindowPos(default_position, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(size);

	m_bOpen = ImGui::Begin(name, nullptr, flags);

	if (!m_bOpen)
		return false;

	m_pDrawList = ImGui::GetWindowDrawList();
	m_Position = ImGui::GetWindowPos();

	const ImVec2 end = m_Position + size;

	m_pDrawList->PushClipRect(m_Position - ImVec2(14.f, 3.f),
		ImVec2(end.x + 3.f, m_Position.y + ImMax(size.y, clip_height) + 3.f), false);

	const float fill = ImMax(opacity, gear * open);
	const float border = ImMax(opacity, gear * 0.85f);

	if (fill > 0.004f)
		m_pDrawList->AddRectFilled(m_Position, end, GetColor(c::background::bg, fill), c::child::rounding);

	if (border > 0.004f)
		m_pDrawList->AddRect(m_Position + ImVec2(0.5f, 0.5f), end - ImVec2(0.5f, 0.5f), GetColor(c::child::border, border), c::child::rounding);

	return true;
}

void CPanel::End() {
	if (m_bOpen)
		m_pDrawList->PopClipRect();

	ImGui::End();
	ImGui::PopStyleVar(2);
}
bool CPanel::Header(const std::string& title, const std::string& value, const ImVec4& value_color, bool separator, float gear, float spin, float t) {
	const float h = HEADER_HEIGHT * t;
	const float ta = ImClamp((t - 0.4f) * 1.7f, 0.f, 1.f);
	const float text_y = m_flCursor + (h - font::tab->FontSize) * 0.5f;
	const float room = COG_SPACE * gear;
	const float icon = gear * t;

	bool clicked = false;

	if (ta > 0.01f) {
		PutText(title, PADDING, text_y, col_title, ta);

		if (!value.empty())
			PutText(value, m_flWidth - PADDING - room - TextWidth(value), text_y, value_color, ta);
	}

	if (icon > 0.015f) {
		bool hovered = false;

		if (icon > 0.99f && HitBox("##cog", m_flWidth - PADDING - 15.f, m_flCursor + 3.f, 20.f, h - 6.f)) {
			hovered = ImGui::IsItemHovered();
			clicked = ImGui::IsItemClicked();
		}

		const float hover = StepAnim("##cog_hover", hovered ? 1.f : 0.f, 13.f);

		DrawGear(m_pDrawList, m_Position + ImVec2(m_flWidth - PADDING - 5.5f, m_flCursor + h * 0.5f),
			6.f, spin, GetColor(Lerp4(col_muted, c::accent, hover), icon));
	}

	m_flCursor += h;

	if (separator) {
		if (ta > 0.01f)
			m_pDrawList->AddLine(m_Position + ImVec2(1.f, m_flCursor), m_Position + ImVec2(m_flWidth - 1.f, m_flCursor), GetColor(c::child::border, ta));

		m_flCursor += t;
	}

	return clicked;
}

void CPanel::Separator(float t) {
	if (t > 0.015f)
		m_pDrawList->AddLine(m_Position + ImVec2(1.f, m_flCursor), m_Position + ImVec2(m_flWidth - 1.f, m_flCursor), GetColor(c::child::border, t));

	m_flCursor += t;
}

void CPanel::Bar(float fraction, const ImVec4& color, float t) {
	const float h = BAR_HEIGHT * t;

	if (h > 0.4f) {
		const ImVec2 start = m_Position + ImVec2(PADDING, m_flCursor);
		const ImVec2 end = start + ImVec2(m_flWidth - PADDING * 2.f, h);

		m_pDrawList->AddRectFilled(start, end, GetColor(c::child::bg, t), h * 0.5f);
		m_pDrawList->AddRectFilled(start, ImVec2(start.x + (end.x - start.x) * ImClamp(fraction, 0.f, 1.f), end.y), GetColor(color, t), h * 0.5f);
	}

	m_flCursor += h;
}
void CPanel::Row(const std::string& title, const std::string& value, const std::string& extra, const ImVec4& title_color, const ImVec4& value_color, float t, float indent) {
	const float h = ROW_HEIGHT * t;
	const float ta = ImClamp((t - 0.4f) * 1.7f, 0.f, 1.f);

	if (ta > 0.01f) {
		const float text_y = m_flCursor + (h - font::tab->FontSize) * 0.5f;

		float right = m_flWidth - PADDING;

		if (!value.empty()) {
			right -= TextWidth(value);
			PutText(value, right, text_y, value_color, ta);
			right -= 7.f;
		}

		if (!extra.empty()) {
			right -= TextWidth(extra);
			PutText(extra, right, text_y, col_muted, ta);
		}

		const float title_x = PADDING + indent;

		PutText(Shorten(title, ImMax(right - title_x - 6.f, 16.f)), title_x, text_y, title_color, ta);
	}

	m_flCursor += h;
}

bool CPanel::RowEye(int index, bool interactive, const std::string& title, const std::string& value, const std::string& extra, const ImVec4& title_color, const ImVec4& value_color, const ImVec4& eye_color, float t, float gear, float slash, float& hover) {
	const float start = m_flCursor;
	const float h = ROW_HEIGHT * t;

	bool clicked = false;

	if (gear > 0.015f && t > 0.015f) {
		bool hovered = false;

		ImGui::PushID(index);

		if (interactive && gear > 0.99f && t > 0.99f && HitBox("##eye", PADDING - 4.f, start + 1.f, EYE_SPACE + 3.f, h - 2.f)) {
			hovered = ImGui::IsItemHovered();
			clicked = ImGui::IsItemClicked();
		}

		ImGui::PopID();

		Approach(hover, hovered ? 1.f : 0.f, 13.f);

		DrawEye(m_pDrawList, m_Position + ImVec2(PADDING + 6.3f, start + h * 0.5f), slash,
			GetColor(Lerp4(eye_color, c::accent, hover), gear * t), GetColor(c::background::bg, gear * t));
	}

	Row(title, value, extra, title_color, value_color, t, EYE_SPACE * gear);

	return clicked;
}
void CPanel::Toggle(const char* label, CCheckBox* box, float t) {
	const float h = SETTINGS_ROW * t;
	constexpr float side = 11.f;

	char key[64];
	ImFormatString(key, IM_ARRAYSIZE(key), "%s##c", label);

	bool hovered = false;

	if (t > 0.99f && HitBox(label, PADDING - 3.f, m_flCursor, m_flWidth - PADDING * 2.f + 6.f, h)) {
		hovered = ImGui::IsItemHovered();

		if (ImGui::IsItemClicked()) {
			box->value = !box->value;

			for (auto cb : box->callbacks)
				cb();

			for (auto& lcb : box->lua_callbacks)
				lcb.func();
		}
	}

	const float hover = StepAnim(label, hovered ? 1.f : 0.f, 13.f);
	const float check = StepAnim(key, box->value ? 1.f : 0.f, 15.f);

	if (t > 0.015f) {
		const float ta = ImClamp((t - 0.4f) * 1.7f, 0.f, 1.f);

		if (ta > 0.01f)
			PutText(label, PADDING, m_flCursor + (h - font::tab->FontSize) * 0.5f, Lerp4(col_muted, col_value, ImMax(check, hover)), ta);

		const ImVec2 min = m_Position + ImVec2(m_flWidth - PADDING - side, m_flCursor + (h - side) * 0.5f);
		const ImVec2 max = min + ImVec2(side, side);

		m_pDrawList->AddRectFilled(min, max, GetColor(Lerp4(c::checkbox::i_bg, c::checkbox::i_bg_hov, hover), t), c::checkbox::rounding);
		m_pDrawList->AddRect(min + ImVec2(0.5f, 0.5f), max - ImVec2(0.5f, 0.5f), GetColor(c::child::border, t), c::checkbox::rounding);

		if (check > 0.01f)
			DrawCheck(m_pDrawList, (min + max) * 0.5f, check, GetColor(c::accent, t));
	}

	m_flCursor += h;
}
void CPanel::Slider(const char* label, CSliderInt* slider, const char* suffix, float t) {
	const float h = SETTINGS_ROW * t;
	const float x0 = m_flWidth * 0.47f;
	const float x1 = m_flWidth - PADDING - 34.f;

	bool grabbed = false;

	if (t > 0.99f && HitBox(label, x0 - 7.f, m_flCursor, x1 - x0 + 14.f, h)) {
		grabbed = ImGui::IsItemActive();

		if (grabbed && x1 > x0) {
			const float frac = ImClamp((ImGui::GetIO().MousePos.x - (m_Position.x + x0)) / (x1 - x0), 0.f, 1.f);
			const int wanted = slider->min + static_cast<int>(frac * (slider->max - slider->min) + 0.5f);

			if (wanted != slider->value) {
				slider->value = wanted;

				for (auto cb : slider->callbacks)
					cb();

				for (auto& lcb : slider->lua_callbacks)
					lcb.func();
			}
		}
		else
			grabbed = ImGui::IsItemHovered();
	}

	const float hover = StepAnim(label, grabbed ? 1.f : 0.f, 13.f);

	if (t > 0.015f) {
		const float ta = ImClamp((t - 0.4f) * 1.7f, 0.f, 1.f);
		const float range = static_cast<float>(slider->max - slider->min);
		const float frac = range > 0.f ? (slider->value - slider->min) / range : 0.f;
		const float cy = m_flCursor + h * 0.5f;

		if (ta > 0.01f) {
			const std::string text = std::format("{}{}", slider->value, suffix);

			PutText(label, PADDING, m_flCursor + (h - font::tab->FontSize) * 0.5f, col_value, ta);
			PutText(text, m_flWidth - PADDING - TextWidth(text), m_flCursor + (h - font::tab->FontSize) * 0.5f, Lerp4(col_muted, c::accent, hover), ta);
		}

		const ImVec2 min = m_Position + ImVec2(x0, cy - 1.5f);
		const ImVec2 max = m_Position + ImVec2(x1, cy + 1.5f);
		const float fill = min.x + (max.x - min.x) * frac;

		m_pDrawList->AddRectFilled(min, max, GetColor(c::slider::i_bg, t), 1.5f);
		m_pDrawList->AddRectFilled(min, ImVec2(fill, max.y), GetColor(c::accent, t), 1.5f);
		m_pDrawList->AddCircleFilled(ImVec2(fill, cy), 3.f + hover, GetColor(c::slider::circle, t), 12);
	}

	m_flCursor += h;
}
void CPanel::ColorRow(const char* label, CColorPicker* picker, float t) {
	const float h = SETTINGS_ROW * t;
	constexpr float sw = 26.f;
	constexpr float sh = 11.f;
	const float x = m_flWidth - PADDING - sw;

	char popup[64];
	ImFormatString(popup, IM_ARRAYSIZE(popup), "%s##p", label);

	bool hovered = false;

	if (t > 0.99f && HitBox(label, x - 5.f, m_flCursor, sw + 10.f, h)) {
		hovered = ImGui::IsItemHovered();

		if (ImGui::IsItemClicked()) {
			GImGui->ColorPickerRef = Pick(picker);
			ImGui::OpenPopup(popup);
		}
	}

	const float hover = StepAnim(label, hovered ? 1.f : 0.f, 13.f);

	if (t > 0.015f) {
		const float ta = ImClamp((t - 0.4f) * 1.7f, 0.f, 1.f);
		const ImVec4 color = Pick(picker);

		if (ta > 0.01f)
			PutText(label, PADDING, m_flCursor + (h - font::tab->FontSize) * 0.5f, col_value, ta);

		const ImVec2 min = m_Position + ImVec2(x, m_flCursor + (h - sh) * 0.5f);
		const ImVec2 max = min + ImVec2(sw, sh);

		m_pDrawList->AddRectFilled(min, max, GetColor(ImVec4(color.x, color.y, color.z, 1.f), t), c::picker::rounding);

		if (color.w < 0.99f)
			m_pDrawList->AddRectFilled(ImVec2(min.x + (max.x - min.x) * color.w, max.y - 2.f), ImVec2(max.x, max.y),
				GetColor(ImVec4(0.f, 0.f, 0.f, 0.65f), t), 0.f);

		m_pDrawList->AddRect(min - ImVec2(0.5f, 0.5f), max + ImVec2(0.5f, 0.5f), GetColor(Lerp4(c::child::border, c::accent, hover), t), c::picker::rounding);
	}

	m_flCursor += h;

	ImGui::PushStyleColor(ImGuiCol_PopupBg, c::picker::i_bg);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, c::picker::rounding);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, g_StylePadding);

	if (ImGui::BeginPopup(popup)) {
		if (t < 0.99f)
			ImGui::CloseCurrentPopup();
		else {
			ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 15.f);
			ImGui::ColorPicker4("##picker", picker->value, ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel
				| ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar, &GImGui->ColorPickerRef.x);
		}

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}
void CElements::DrawBombTimer() {
	CPlantedC4* bomb = nullptr;

	if (Cheat.InGame) {
		CBaseEntity* cached = m_nBombIndex == -1 ? nullptr : EntityList->GetClientEntity(m_nBombIndex);
		ClientClass* cached_class = cached ? cached->GetClientClass() : nullptr;

		if (cached_class && cached_class->m_ClassID == C_PLANTED_C4)
			bomb = reinterpret_cast<CPlantedC4*>(cached);
		else if (GlobalVars->realtime > m_flNextBombScan) {
			m_nBombIndex = -1;
			m_flNextBombScan = GlobalVars->realtime + 0.25f;

			for (int i = ClientState->m_nMaxClients + 1; i < EntityList->GetHighestEntityIndex(); i++) {
				CBaseEntity* entity = EntityList->GetClientEntity(i);

				if (!entity)
					continue;

				ClientClass* entity_class = entity->GetClientClass();

				if (!entity_class || entity_class->m_ClassID != C_PLANTED_C4)
					continue;

				m_nBombIndex = i;
				bomb = reinterpret_cast<CPlantedC4*>(entity);
				break;
			}
		}
	}
	else
		m_nBombIndex = -1;

	bomb_state_t state;

	if (bomb && !bomb->m_bBombDefused() && bomb->m_flC4Blow() > GlobalVars->curtime) {
		state.live = true;
		state.remaining = bomb->m_flC4Blow() - GlobalVars->curtime;
		state.timer_length = bomb->m_flTimerLength() > 0.f ? bomb->m_flTimerLength() : 40.f;

		if (EntityList->GetClientEntityFromHandle(bomb->m_hBombDefuser())) {
			state.defusing = true;
			state.defuse_remaining = ImMax(bomb->m_flDefuseCountDown() - GlobalVars->curtime, 0.f);
			state.can_defuse = bomb->m_flDefuseCountDown() <= bomb->m_flC4Blow();
		}
	}

	const bool menu_open = Menu->IsOpened();

	if (state.live || menu_open)
		m_BombState = state;

	panel_state_t& panel_state = m_BombPanel;

	if (!menu_open)
		panel_state.settings = false;

	Fade(panel_state.gear, menu_open);
	Approach(panel_state.open, panel_state.settings ? 1.f : 0.f, 12.f);
	Approach(panel_state.spin, panel_state.spin_target, 9.f);

	Fade(panel_state.alpha, config.visuals.elements.bomb_timer->get() && (state.live || menu_open));

	const float bar = Eased(m_flBombBar, config.visuals.elements.bomb_progress->get() ? 1.f : 0.f, 12.f);
	const float defuse = Eased(m_flBombDefuse, m_BombState.defusing && config.visuals.elements.bomb_defuse->get() ? 1.f : 0.f, 12.f);

	if (panel_state.alpha <= 0.01f)
		return;
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	const float opacity = config.visuals.elements.bomb_opacity->get() * 0.01f;
	const ImVec4 active_col = Pick(config.visuals.elements.bomb_active);
	const ImVec4 inactive_col = Pick(config.visuals.elements.bomb_inactive);

	const float cf = ImMax(bar, defuse);
	const float presence = ImClamp(cf * 4.f, 0.f, 1.f);
	const float body = 14.f * cf + BAR_HEIGHT * bar + 5.f * bar * defuse + ROW_HEIGHT * defuse;
	const float settings = (presence + 12.f + SETTINGS_ROW * 5.f) * panel_state.open;
	const bool divider = presence > 0.01f || panel_state.open > 0.01f;
	const float height = HEADER_HEIGHT + (divider ? 1.f : 0.f) + body + settings;

	CPanel panel;

	if (panel.Begin("##lambda_bomb", ImVec2(20.f, display.y * 0.45f), ImVec2(PANEL_WIDTH, height),
		panel_state.alpha, opacity, panel_state.gear, panel_state.open)) {

		const std::string value = m_BombState.live ? std::format("{:.1f}s", m_BombState.remaining) : "--";
		const ImVec4 value_col = m_BombState.live ? (m_BombState.remaining < 10.f ? col_bad : active_col) : inactive_col;

		if (panel.Header("bomb", value, value_col, divider, panel_state.gear, panel_state.spin)) {
			panel_state.settings = !panel_state.settings;
			panel_state.spin_target += 2.0944f;
		}

		panel.Pad(7.f * cf);

		const float fraction = m_BombState.timer_length > 0.f ? m_BombState.remaining / m_BombState.timer_length : 0.f;
		const ImVec4 bar_col = m_BombState.defusing ? (m_BombState.can_defuse ? col_good : col_bad) : active_col;

		panel.Bar(fraction, bar_col, bar);
		panel.Pad(5.f * bar * defuse);
		panel.Row("defuse", std::format("{:.1f}s", m_BombState.defuse_remaining), "", inactive_col,
			m_BombState.can_defuse ? col_good : col_bad, defuse);
		panel.Pad(7.f * cf);

		panel.Separator(panel_state.open * presence);
		panel.Pad(6.f * panel_state.open);
		panel.Slider("opacity", config.visuals.elements.bomb_opacity, "%", panel_state.open);
		panel.ColorRow("active", config.visuals.elements.bomb_active, panel_state.open);
		panel.ColorRow("inactive", config.visuals.elements.bomb_inactive, panel_state.open);
		panel.Toggle("progress bar", config.visuals.elements.bomb_progress, panel_state.open);
		panel.Toggle("defuse timer", config.visuals.elements.bomb_defuse, panel_state.open);
	}

	panel.End();
}
void CElements::DrawKeybinds() {
	const bool menu_open = Menu->IsOpened();
	const bool show_all = config.visuals.elements.keybinds_show_all->get();
	const bool show_key = config.visuals.elements.keybinds_show_key->get();
	const bool show_mode = config.visuals.elements.keybinds_show_mode->get();

	panel_state_t& panel_state = m_KeybindPanel;

	if (!menu_open)
		panel_state.settings = false;

	Fade(panel_state.gear, menu_open);
	Approach(panel_state.open, panel_state.settings ? 1.f : 0.f, 12.f);
	Approach(panel_state.spin, panel_state.spin_target, 9.f);

	for (auto& row : m_KeybindRows)
		row.present = false;

	for (IBaseWidget* widget : Menu->GetKeyBinds()) {
		CKeyBind* bind = reinterpret_cast<CKeyBind*>(widget);

		if (!bind->visible || bind->key == 0)
			continue;

		if (bind->parent_item && bind->parent_item->GetType() == WidgetType::Checkbox
			&& !reinterpret_cast<CCheckBox*>(bind->parent_item)->get(false))
			continue;

		bool active = false;
		std::string mode;

		if (bind->mode == 2) {
			active = true;
			mode = "always";
		}
		else if (bind->mode == 1) {
			active = bind->toggled;
			mode = "toggle";
		}
		else {
			active = !ctx.KeysBlocked() && (GetAsyncKeyState(bind->key) & 0x8000);
			mode = "hold";
		}

		if (!menu_open && (bind->hidden || (!active && !show_all)))
			continue;

		const uintptr_t id = reinterpret_cast<uintptr_t>(bind);

		anim_row_t* row = nullptr;

		for (auto& existing : m_KeybindRows) {
			if (existing.id != id)
				continue;

			row = &existing;
			break;
		}

		if (!row) {
			m_KeybindRows.emplace_back();
			row = &m_KeybindRows.back();
			row->id = id;
		}

		row->title = Lower(bind->name);
		row->value = show_mode ? mode : "";
		row->extra = show_key ? KeyName(bind->key) : "";
		row->active = active;
		row->hidden = bind->hidden;
		row->present = true;
	}

	float total = 0.f;

	for (auto it = m_KeybindRows.begin(); it != m_KeybindRows.end();) {
		Fade(it->alpha, it->present);
		Approach(it->slash, it->hidden ? 1.f : 0.f, 11.f);

		if (!it->present && it->alpha <= 0.001f) {
			it = m_KeybindRows.erase(it);
			continue;
		}

		total += ROW_HEIGHT * it->alpha;
		it++;
	}

	Fade(panel_state.alpha, config.visuals.elements.keybinds->get() && (menu_open || total > 0.01f));

	if (panel_state.alpha <= 0.01f)
		return;
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	const float opacity = config.visuals.elements.keybinds_opacity->get() * 0.01f;
	const ImVec4 active_col = Pick(config.visuals.elements.keybinds_active);
	const ImVec4 inactive_col = Pick(config.visuals.elements.keybinds_inactive);

	const float rf = ImClamp(total / ROW_HEIGHT, 0.f, 1.f);
	const float settings = (rf + 12.f + SETTINGS_ROW * 6.f) * panel_state.open;
	const bool divider = rf > 0.01f || panel_state.open > 0.01f;
	const float height = HEADER_HEIGHT + (divider ? 1.f : 0.f) + total + 14.f * rf + settings;

	CPanel panel;

	if (panel.Begin("##lambda_keybinds", ImVec2(20.f, display.y - 260.f), ImVec2(PANEL_WIDTH, height),
		panel_state.alpha, opacity, panel_state.gear, panel_state.open)) {

		if (panel.Header("keybinds", "", col_muted, divider, panel_state.gear, panel_state.spin)) {
			panel_state.settings = !panel_state.settings;
			panel_state.spin_target += 2.0944f;
		}

		panel.Pad(7.f * rf);

		for (auto& row : m_KeybindRows) {
			const bool dim = row.hidden || !row.active;

			if (panel.RowEye(static_cast<int>(row.id >> 4), row.present, row.title, row.value, row.extra,
				dim ? inactive_col : col_title, dim ? inactive_col : active_col,
				row.hidden ? inactive_col : col_muted, row.alpha, panel_state.gear, row.slash, row.hover)) {

				CKeyBind* bind = reinterpret_cast<CKeyBind*>(row.id);

				bind->hidden = !bind->hidden;
				row.hidden = bind->hidden;
			}
		}

		panel.Pad(7.f * rf);

		panel.Separator(panel_state.open * rf);
		panel.Pad(6.f * panel_state.open);
		panel.Slider("opacity", config.visuals.elements.keybinds_opacity, "%", panel_state.open);
		panel.ColorRow("active", config.visuals.elements.keybinds_active, panel_state.open);
		panel.ColorRow("inactive", config.visuals.elements.keybinds_inactive, panel_state.open);
		panel.Toggle("show all", config.visuals.elements.keybinds_show_all, panel_state.open);
		panel.Toggle("show key", config.visuals.elements.keybinds_show_key, panel_state.open);
		panel.Toggle("show mode", config.visuals.elements.keybinds_show_mode, panel_state.open);
	}

	panel.End();
}
void CElements::DrawSpectators() {
	const bool menu_open = Menu->IsOpened();
	const bool show_mode = config.visuals.elements.spectators_show_mode->get();

	panel_state_t& panel_state = m_SpectatorPanel;

	if (!menu_open)
		panel_state.settings = false;

	Fade(panel_state.gear, menu_open);
	Approach(panel_state.open, panel_state.settings ? 1.f : 0.f, 12.f);
	Approach(panel_state.spin, panel_state.spin_target, 9.f);

	for (auto& row : m_SpectatorRows)
		row.present = false;

	int count = 0;

	if (Cheat.InGame && Cheat.LocalPlayer) {
		CBaseEntity* target = Cheat.LocalPlayer;

		if (!Cheat.LocalPlayer->IsAlive()) {
			CBaseEntity* observed = EntityList->GetClientEntityFromHandle(Cheat.LocalPlayer->m_hObserverTarget());

			if (observed)
				target = observed;
		}

		for (int i = 1; i <= ClientState->m_nMaxClients; i++) {
			CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));

			if (!player || player == Cheat.LocalPlayer)
				continue;

			const int mode = player->m_iObserverMode();

			if (mode != OBS_MODE_IN_EYE && mode != OBS_MODE_CHASE)
				continue;

			if (EntityList->GetClientEntityFromHandle(player->m_hObserverTarget()) != target)
				continue;

			const uintptr_t id = static_cast<uintptr_t>(i);

			anim_row_t* row = nullptr;

			for (auto& existing : m_SpectatorRows) {
				if (existing.id != id)
					continue;

				row = &existing;
				break;
			}

			if (!row) {
				m_SpectatorRows.emplace_back();
				row = &m_SpectatorRows.back();
				row->id = id;
			}

			row->title = player->GetName();
			row->value = show_mode ? (mode == OBS_MODE_IN_EYE ? "first" : "third") : "";
			row->active = mode == OBS_MODE_IN_EYE;
			row->present = true;

			count++;
		}
	}

	float total = 0.f;

	for (auto it = m_SpectatorRows.begin(); it != m_SpectatorRows.end();) {
		Fade(it->alpha, it->present);

		if (!it->present && it->alpha <= 0.001f) {
			it = m_SpectatorRows.erase(it);
			continue;
		}

		total += ROW_HEIGHT * it->alpha;
		it++;
	}

	Fade(panel_state.alpha, config.visuals.elements.spectators->get() && (menu_open || total > 0.01f));

	if (panel_state.alpha <= 0.01f)
		return;
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	const float opacity = config.visuals.elements.spectators_opacity->get() * 0.01f;
	const ImVec4 active_col = Pick(config.visuals.elements.spectators_active);
	const ImVec4 inactive_col = Pick(config.visuals.elements.spectators_inactive);

	const float rf = ImClamp(total / ROW_HEIGHT, 0.f, 1.f);
	const float settings = (rf + 12.f + SETTINGS_ROW * 4.f) * panel_state.open;
	const bool divider = rf > 0.01f || panel_state.open > 0.01f;
	const float height = HEADER_HEIGHT + (divider ? 1.f : 0.f) + total + 14.f * rf + settings;

	CPanel panel;

	if (panel.Begin("##lambda_spectators", ImVec2(display.x - PANEL_WIDTH - 20.f, 20.f), ImVec2(PANEL_WIDTH, height),
		panel_state.alpha, opacity, panel_state.gear, panel_state.open)) {

		const std::string value = count > 0 ? std::format("{}", count) : "";

		if (panel.Header("spectators", value, active_col, divider, panel_state.gear, panel_state.spin)) {
			panel_state.settings = !panel_state.settings;
			panel_state.spin_target += 2.0944f;
		}

		panel.Pad(7.f * rf);

		for (auto& row : m_SpectatorRows)
			panel.Row(row.title, row.value, "", row.active ? col_title : inactive_col,
				row.active ? active_col : inactive_col, row.alpha);

		panel.Pad(7.f * rf);

		panel.Separator(panel_state.open * rf);
		panel.Pad(6.f * panel_state.open);
		panel.Slider("opacity", config.visuals.elements.spectators_opacity, "%", panel_state.open);
		panel.ColorRow("active", config.visuals.elements.spectators_active, panel_state.open);
		panel.ColorRow("inactive", config.visuals.elements.spectators_inactive, panel_state.open);
		panel.Toggle("show mode", config.visuals.elements.spectators_show_mode, panel_state.open);
	}

	panel.End();
}
void CElements::DrawEventLog() {
	const bool menu_open = Menu->IsOpened();
	const float lifetime = static_cast<float>(config.visuals.elements.log_duration->get());
	const float now = static_cast<float>(ImGui::GetTime());

	panel_state_t& panel_state = m_LogPanel;

	if (!menu_open)
		panel_state.settings = false;

	Fade(panel_state.gear, menu_open);
	Approach(panel_state.open, panel_state.settings ? 1.f : 0.f, 12.f);
	Approach(panel_state.spin, panel_state.spin_target, 9.f);

	std::vector<std::string> pending;

	{
		std::lock_guard<std::mutex> lock(m_LogMutex);
		pending.swap(m_PendingLogs);
	}

	for (const std::string& text : pending) {
		log_t entry;
		entry.birth = now;

		for (const log_segment_t& segment : ParseLogSegments(text, config.visuals.elements.log_text->get(), config.visuals.elements.log_accent->get())) {
			log_part_t part;

			part.text = segment.text;
			part.color = ToVec4(segment.color);
			part.width = TextWidth(segment.text);

			entry.width += part.width;
			entry.parts.push_back(part);
		}

		if (entry.parts.empty())
			continue;

		m_Logs.push_back(entry);

		while (m_Logs.size() > LOG_LIMIT)
			m_Logs.pop_front();
	}

	while (!m_Logs.empty() && now - m_Logs.front().birth > lifetime + LOG_FADE)
		m_Logs.pop_front();

	Fade(panel_state.alpha, config.visuals.elements.event_log->get() && (menu_open || !m_Logs.empty()));

	if (panel_state.alpha <= 0.01f)
		return;
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	const float opacity = config.visuals.elements.log_opacity->get() * 0.01f;

	float widest = 60.f;

	for (const log_t& entry : m_Logs)
		widest = ImMax(widest, entry.width);

	const float natural = ImMax(static_cast<float>(m_Logs.size()), 1.f) * LOG_ROW_HEIGHT;
	const float reserved = static_cast<float>(LOG_LIMIT) * LOG_ROW_HEIGHT;
	const float rows_h = natural + (reserved - natural) * panel_state.gear;
	const float head = (HEADER_HEIGHT + 1.f) * panel_state.gear;
	const float settings = (13.f + SETTINGS_ROW * 4.f) * panel_state.open;

	const float wanted = ImMax(widest + 12.f + PADDING * panel_state.gear, PANEL_WIDTH * panel_state.gear);

	if (m_flLogWidth <= 0.f)
		m_flLogWidth = wanted;

	const float width = Eased(m_flLogWidth, wanted, 13.f);

	CPanel panel;

	if (panel.Begin("##lambda_log", ImVec2(20.f, display.y - 430.f), ImVec2(width, head + rows_h + settings),
		panel_state.alpha, opacity, panel_state.gear, panel_state.open, head + reserved + settings)) {

		if (panel.Header("event log", "", col_muted, true, panel_state.gear, panel_state.spin, panel_state.gear)) {
			panel_state.settings = !panel_state.settings;
			panel_state.spin_target += 2.0944f;
		}

		const float base_x = 4.f + (PADDING - 4.f) * panel_state.gear;
		const float top = panel.Cursor();
		const float step = ImMin(ImGui::GetIO().DeltaTime * 14.f, 1.f);

		int index = 0;

		for (log_t& entry : m_Logs) {
			const float target = index * LOG_ROW_HEIGHT;

			if (!entry.placed) {
				entry.y = target;
				entry.placed = true;
			}
			else
				entry.y += (target - entry.y) * step;

			entry.x -= entry.x * step;

			const float age = now - entry.birth;
			const float fade = age > lifetime ? ImClamp(1.f - (age - lifetime) / LOG_FADE, 0.f, 1.f) : 1.f;
			const float alpha = fade * ImClamp(1.f + entry.x / 12.f, 0.f, 1.f);

			index++;

			if (alpha <= 0.01f)
				continue;

			float x = base_x + entry.x;
			const float y = top + entry.y + (LOG_ROW_HEIGHT - font::tab->FontSize) * 0.5f;

			for (const log_part_t& part : entry.parts) {
				panel.PutText(part.text, x + 1.f, y + 1.f, ImVec4(0.f, 0.f, 0.f, 0.7f), alpha);
				panel.PutText(part.text, x, y, part.color, alpha);

				x += part.width;
			}
		}

		panel.Pad(rows_h);

		panel.Separator(panel_state.open);
		panel.Pad(6.f * panel_state.open);
		panel.Slider("opacity", config.visuals.elements.log_opacity, "%", panel_state.open);
		panel.ColorRow("accent", config.visuals.elements.log_accent, panel_state.open);
		panel.ColorRow("text", config.visuals.elements.log_text, panel_state.open);
		panel.Slider("duration", config.visuals.elements.log_duration, "s", panel_state.open);
	}

	panel.End();
}
void CElements::AddLog(const std::string& msg) {
	if (msg.empty() || !config.visuals.elements.event_log || !config.visuals.elements.event_log->get())
		return;

	std::lock_guard<std::mutex> lock(m_LogMutex);

	if (m_PendingLogs.size() >= LOG_LIMIT * 4)
		m_PendingLogs.erase(m_PendingLogs.begin());

	m_PendingLogs.push_back(msg);
}

void CElements::Draw() {
	if (!font::tab || !config.visuals.elements.log_duration)
		return;

	DrawBombTimer();
	DrawKeybinds();
	DrawSpectators();
	DrawEventLog();
}
