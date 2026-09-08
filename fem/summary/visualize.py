#!/usr/bin/env python3
"""
fem.csv インタラクティブ可視化ダッシュボード生成器 (fem アプリ専用)

使い方:
    python3 visualize.py [csv_path] [out_html]

    csv_path: default = ./fem.csv (同一ディレクトリ)
    out_html: default = ./fem_dashboard.html (同一ディレクトリ)

ブラウザで開くと、左サイドバーで variant / impl / machine / mode などをフィルタし、
X 軸 / Y 軸 / 色分け / グラフ種別 (散布 / 折れ線 / 棒 / 箱ひげ / ヒストグラム) を
自由に切り替えて性能比較できる。

注: diffusion 版とは独立 (diffusion/summary/visualize.py を参照しない)。
    CSV スキーマは diffusion と同じ 21 列だが、列の意味解釈が fem 固有:
      N        = NP (節点数, DOF)
      error    = CG 最終残差
      (last_sim_time, num_time_steps_logged 等は fem 流儀で出力)
"""
import json
import math
import os
import sys
import webbrowser

import pandas as pd

ROOT = os.path.dirname(os.path.abspath(__file__))
CSV_PATH = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "fem.csv")
OUT_HTML = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "fem_dashboard.html")


# ============================================================
# 1. データ読み込みと前処理
# ============================================================
print(f"読み込み中: {CSV_PATH}")
df = pd.read_csv(CSV_PATH, encoding="utf-8-sig")
print(f"  行数: {len(df)},  列数: {len(df.columns)}")

# 数値化 (空文字や NaN は欠損として扱う)
NUMERIC_COLS = [
    "fp", "N",
    "time_sec", "performance_gflops", "error",
    "real_sec", "user_sec", "sys_sec",
]
for c in NUMERIC_COLS:
    if c in df.columns:
        df[c] = pd.to_numeric(df[c], errors="coerce")

# 表示用ラベル化: 空の variant / optimization_param をわかりやすく
df["variant_display"] = df["variant"].fillna("").replace("", "(no variant)")
df["param_display"] = df["optimization_param"].fillna("").replace("", "(baseline)")

print(f"  machine     : {sorted(df['machine'].dropna().unique())}")
print(f"  impl        : {sorted(df['impl'].dropna().unique())}")
print(f"  opt_type    : {sorted(df['optimization_type'].dropna().unique())}")


# ============================================================
# 2. 軸/フィルタ/色分けの選択肢
# ============================================================
AXIS_CHOICES = [c for c in NUMERIC_COLS if c in df.columns and df[c].notna().any()]

# フィルタ軸から外した列 (いずれも他の軸から一意に決まり絞り込みの幅を増やさない):
#   category          … impl と重複 (Kokkos / non-Kokkos は impl で判る)
#   optimization_type … (impl, variant) から一意に決まる
# CSV の列自体は互換性のため残し、色分け軸 (COLOR_CATS) にも残す
# (baseline / tile-sweep 等でまとめて色を付けられると便利なため)。
FILTER_CATS = [
    c for c in [
        "machine", "mode", "language",
        "impl", "variant_display", "memory_model",
        "param_display", "fp",
    ]
    if c in df.columns
]

COLOR_CATS = [
    "impl", "variant_display", "memory_model", "optimization_type",
    "machine", "mode", "language", "fp", "param_display",
]

PALETTE = [
    "#636EFA", "#EF553B", "#00CC96", "#AB63FA", "#FFA15A",
    "#19D3F3", "#FF6692", "#B6E880", "#FF97FF", "#FECB52",
    "#1F77B4", "#FF7F0E", "#2CA02C", "#D62728", "#9467BD",
    "#8C564B", "#E377C2", "#7F7F7F", "#BCBD22", "#17BECF",
]


def build_dashboard_html():
    records = df.to_dict(orient="records")

    def clean_val(v):
        if isinstance(v, float) and math.isnan(v):
            return None
        return v

    clean_records = [{k: clean_val(v) for k, v in r.items()} for r in records]
    data_json = json.dumps(clean_records, ensure_ascii=False)

    def default_sel(c, default):
        return "selected" if c == default else ""

    axis_x_options_html = "\n".join(
        f'<option value="{c}" {default_sel(c, "N")}>{c}</option>'
        for c in AXIS_CHOICES
    )
    axis_y_options_html = "\n".join(
        f'<option value="{c}" {default_sel(c, "performance_gflops")}>{c}</option>'
        for c in AXIS_CHOICES
    )
    color_options_html = "\n".join(
        f'<option value="{c}" {default_sel(c, "impl")}>{c}</option>'
        for c in COLOR_CATS
    )
    chart_options_html = """
        <option value="scatter" selected>散布図 (Scatter)</option>
        <option value="line">折れ線 (Line)</option>
        <option value="bar">棒グラフ (Bar)</option>
        <option value="box">箱ひげ図 (Box)</option>
        <option value="histogram">ヒストグラム (Histogram)</option>
    """
    scale_options_html = """
        <option value="linear">線形 (Linear)</option>
        <option value="log" selected>対数 (Log)</option>
    """
    fmt_options_html = """
        <option value="power" selected>10^n 上付き (10⁹)</option>
        <option value="E">E表記 (1E+09)</option>
        <option value="e">e表記 (1e+9)</option>
        <option value="SI">SI接頭辞 (1G, 1T)</option>
        <option value="none">そのまま (1000000000)</option>
    """

    # フィルタ block 生成
    filter_blocks = []
    for cat in FILTER_CATS:
        col = df[cat]
        has_empty = col.isna().any() or (col.astype(str).str.strip() == "").any() \
            or (col.astype(str) == "nan").any()
        vals = sorted(
            v for v in col.dropna().unique().astype(str)
            if v.strip() and v != "nan"
        )
        if len(vals) == 0 or len(vals) > 80:
            continue
        # 各チェックボックスの label には value/count を span で含めて連動フィルタ用に
        checks = "\n".join(
            f'<label data-val="{v}"><input type="checkbox" class="filter-cb" '
            f'data-col="{cat}" value="{v}" checked> <span class="lbl">{v}</span>'
            f'<span class="count"></span></label>'
            for v in vals
        )
        if has_empty:
            checks += (
                f'<label data-val=""><input type="checkbox" class="filter-cb" '
                f'data-col="{cat}" value="" checked> <span class="lbl">(空)</span>'
                f'<span class="count"></span></label>'
            )
        filter_blocks.append(
            f'<div class="filter-group">'
            f'<div class="filter-title">{cat} '
            f'<button class="toggle-btn" onclick="toggleAll(this, \'{cat}\')">'
            f'全解除</button></div>'
            f'<div class="filter-checks">{checks}</div></div>'
        )
    filters_html = "\n".join(filter_blocks)

    html = f"""<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="UTF-8">
<title>fem benchmark dashboard</title>
<script src="https://cdn.plot.ly/plotly-3.0.1.min.js"></script>
<style>
* {{ margin: 0; padding: 0; box-sizing: border-box; }}
body {{ font-family: 'Segoe UI', Arial, sans-serif; background: #f5f5f5; }}
.container {{ display: flex; height: 100vh; }}
.sidebar {{
    width: 460px; min-width: 400px; background: #fff;
    border-right: 2px solid #ddd; overflow-y: auto; padding: 18px;
    font-size: 20px;
}}
.sidebar h2 {{ font-size: 24px; margin-bottom: 15px; color: #333; }}
.sidebar h3 {{ font-size: 20px; margin: 15px 0 9px; color: #444;
              border-bottom: 1px solid #ccc; padding-bottom: 5px; }}
.control-group {{ margin-bottom: 18px; }}
.control-group label {{ font-weight: bold; display: block; margin-bottom: 5px; color: #555; }}
.control-group select {{ width: 100%; padding: 8px; font-size: 20px; }}
.filter-group {{ margin-bottom: 12px; border: 1px solid #eee; border-radius: 4px; padding: 9px; }}
.filter-title {{ font-weight: bold; color: #444; margin-bottom: 6px;
               display: flex; justify-content: space-between; align-items: center; }}
.filter-checks {{ max-height: 195px; overflow-y: auto; padding-left: 6px; }}
.filter-checks label {{ cursor: pointer; white-space: nowrap; font-size: 18px;
                       display: block; padding: 2px 0; }}
.filter-checks label.disabled {{ color: #bbb; cursor: not-allowed; }}
.filter-checks label.disabled input {{ cursor: not-allowed; }}
.filter-checks .count {{ color: #888; font-size: 17px; margin-left: 6px; }}
.filter-checks label.disabled .count {{ color: #ddd; }}
.toggle-btn {{ font-size: 15px; padding: 2px 9px; cursor: pointer;
             background: #eee; border: 1px solid #ccc; border-radius: 3px; }}
.main {{ flex: 1; display: flex; flex-direction: column; }}
#chart {{ flex: 1; }}
.info-bar {{ background: #fff; padding: 12px 24px; border-top: 1px solid #ddd;
           font-size: 18px; color: #666; }}
</style>
</head>
<body>
<div class="container">
  <div class="sidebar">
    <h2>fem dashboard</h2>

    <h3>軸設定</h3>
    <div class="control-group">
      <label>グラフ種別</label>
      <select id="chartType" onchange="updateChart()">{chart_options_html}</select>
    </div>
    <div class="control-group">
      <label>X 軸</label>
      <select id="xAxis" onchange="updateChart()">{axis_x_options_html}</select>
    </div>
    <div class="control-group">
      <label>Y 軸</label>
      <select id="yAxis" onchange="updateChart()">{axis_y_options_html}</select>
    </div>
    <div class="control-group">
      <label>色分け</label>
      <select id="colorBy" onchange="updateChart()">{color_options_html}</select>
    </div>
    <div class="control-group">
      <label>X 軸スケール</label>
      <select id="xScale" onchange="updateChart()">{scale_options_html}</select>
    </div>
    <div class="control-group">
      <label>Y 軸スケール</label>
      <select id="yScale" onchange="updateChart()">{scale_options_html}</select>
    </div>
    <div class="control-group">
      <label>数値表記</label>
      <select id="numFmt" onchange="updateChart()">{fmt_options_html}</select>
    </div>

    <h3>フィルタ</h3>
    {filters_html}
  </div>
  <div class="main">
    <div id="chart"></div>
    <div class="info-bar" id="infoBar">
      左のドロップダウン / チェックボックスを操作するとグラフが更新されます
    </div>
  </div>
</div>

<script>
const DATA = {data_json};
const PALETTE = {json.dumps(PALETTE)};

/** 全 filter group (col name) を列挙 */
function filterCols() {{
    const cols = new Set();
    document.querySelectorAll('.filter-cb').forEach(cb => cols.add(cb.dataset.col));
    return Array.from(cols);
}}

/** 指定 col を除いた全フィルタが許す行を返す (連動カウント用) */
function getFilteredExcept(skipCol) {{
    const allowed = {{}};
    document.querySelectorAll('.filter-cb').forEach(cb => {{
        if (cb.dataset.col === skipCol) return;
        const col = cb.dataset.col;
        if (!allowed[col]) allowed[col] = new Set();
        if (cb.checked) allowed[col].add(cb.value);
    }});
    return DATA.filter(row => {{
        for (const col in allowed) {{
            const val = row[col];
            let s = (val === null || val === undefined) ? "" : String(val);
            if (s === "nan" || s === "NaN") s = "";
            if (!allowed[col].has(s)) return false;
        }}
        return true;
    }});
}}

/** 全フィルタが許す行を返す (チャート表示用) */
function getFilteredData() {{
    const allowed = {{}};
    document.querySelectorAll('.filter-cb').forEach(cb => {{
        const col = cb.dataset.col;
        if (!allowed[col]) allowed[col] = new Set();
        if (cb.checked) allowed[col].add(cb.value);
    }});
    return DATA.filter(row => {{
        for (const col in allowed) {{
            const val = row[col];
            let s = (val === null || val === undefined) ? "" : String(val);
            if (s === "nan" || s === "NaN") s = "";
            if (!allowed[col].has(s)) return false;
        }}
        return true;
    }});
}}

/** 各 filter group のカウントを更新し、0 件のオプションを無効化 */
function updateFilterCounts() {{
    filterCols().forEach(col => {{
        const matching = getFilteredExcept(col);
        const counts = {{}};
        matching.forEach(row => {{
            const val = row[col];
            let s = (val === null || val === undefined) ? "" : String(val);
            if (s === "nan" || s === "NaN") s = "";
            counts[s] = (counts[s] || 0) + 1;
        }});
        document.querySelectorAll(`.filter-cb[data-col="${{col}}"]`).forEach(cb => {{
            const c = counts[cb.value] || 0;
            const label = cb.parentElement;
            const span = label.querySelector('.count');
            if (span) span.textContent = ` (${{c}})`;
            if (c === 0) {{
                cb.disabled = true;
                label.classList.add('disabled');
            }} else {{
                cb.disabled = false;
                label.classList.remove('disabled');
            }}
        }});
    }});
}}

function toggleAll(btn, col) {{
    // 無効化されてない (= 該当データあり) cb のみを対象に
    const cbs = Array.from(document.querySelectorAll(`.filter-cb[data-col="${{col}}"]`))
        .filter(c => !c.disabled);
    const anyChecked = cbs.some(c => c.checked);
    cbs.forEach(c => c.checked = !anyChecked);
    btn.textContent = anyChecked ? "全選択" : "全解除";
    onFilterChange();
}}

function onFilterChange() {{
    updateFilterCounts();
    updateChart();
}}

document.querySelectorAll('.filter-cb').forEach(cb => {{
    cb.addEventListener('change', onFilterChange);
}});

function fmt(v) {{
    if (v === null || v === undefined || v === "") return "—";
    if (typeof v === "number") {{
        if (Math.abs(v) < 1e-3 || Math.abs(v) >= 1e6) return v.toExponential(3);
        return Number(v.toPrecision(4));
    }}
    return v;
}}

function hoverText(r) {{
    return `<b>${{r['impl']}}</b>`
        + (r['variant_display'] && r['variant_display'] !== '(no variant)' ? ` / ${{r['variant_display']}}` : '')
        + (r['param_display'] && r['param_display'] !== '(baseline)' ? ` [${{r['param_display']}}]` : '')
        + `<br>machine=${{r['machine']}}/${{r['mode']}}  lang=${{r['language']}}  fp=${{r['fp']}}<br>`
        + `memory=${{r['memory_model']}}  opt=${{r['optimization_type']}}<br>`
        + `N=${{r['N']}} (NP: 節点数)<br>`
        + `time=${{fmt(r['time_sec'])}}s  perf=${{fmt(r['performance_gflops'])}} GFlop/s<br>`
        + `err=${{fmt(r['error'])}}`;
}}

function updateChart() {{
    const xCol     = document.getElementById('xAxis').value;
    const yCol     = document.getElementById('yAxis').value;
    const colorBy  = document.getElementById('colorBy').value;
    const chartType= document.getElementById('chartType').value;
    const xScale   = document.getElementById('xScale').value;
    const yScale   = document.getElementById('yScale').value;
    const numFmt   = document.getElementById('numFmt').value;

    const filtered = getFilteredData();
    document.getElementById('infoBar').textContent =
        `表示中: ${{filtered.length}} / ${{DATA.length}} 行 | `
        + `X: ${{xCol}} (${{xScale}}) | Y: ${{yCol}} (${{yScale}}) | 色: ${{colorBy}}`;

    const groups = {{}};
    filtered.forEach(row => {{
        const key = (row[colorBy] === null || row[colorBy] === undefined || row[colorBy] === "")
            ? "(空)" : String(row[colorBy]);
        if (!groups[key]) groups[key] = [];
        groups[key].push(row);
    }});

    const traces = [];
    const groupNames = Object.keys(groups).sort();
    groupNames.forEach((name, i) => {{
        const rows = groups[name];
        const color = PALETTE[i % PALETTE.length];
        const hovers = rows.map(hoverText);

        if (chartType === 'scatter') {{
            traces.push({{
                x: rows.map(r => r[xCol]),
                y: rows.map(r => r[yCol]),
                mode: 'markers',
                marker: {{ color: color, size: 7, opacity: 0.75 }},
                name: name,
                text: hovers,
                hoverinfo: 'text',
            }});
        }} else if (chartType === 'line') {{
            const pairs = rows.map(r => [r[xCol], r[yCol], r]);
            pairs.sort((a, b) => (Number(a[0]) || 0) - (Number(b[0]) || 0));
            traces.push({{
                x: pairs.map(p => p[0]),
                y: pairs.map(p => p[1]),
                mode: 'lines+markers',
                marker: {{ color: color, size: 5 }},
                line: {{ color: color, width: 2 }},
                name: name,
                text: pairs.map(p => hoverText(p[2])),
                hoverinfo: 'text',
            }});
        }} else if (chartType === 'bar') {{
            traces.push({{
                x: rows.map(r => r[xCol]),
                y: rows.map(r => r[yCol]),
                type: 'bar',
                marker: {{ color: color, opacity: 0.75 }},
                name: name,
                text: hovers,
                hoverinfo: 'text',
            }});
        }} else if (chartType === 'box') {{
            traces.push({{
                y: rows.map(r => r[yCol]),
                type: 'box',
                marker: {{ color: color }},
                name: name,
            }});
        }} else if (chartType === 'histogram') {{
            traces.push({{
                x: rows.map(r => r[yCol]),
                type: 'histogram',
                marker: {{ color: color, opacity: 0.65 }},
                name: name,
            }});
        }}
    }});

    const layout = {{
        title: {{ text: `${{yCol}}  vs  ${{xCol}}`, font: {{ size: 24 }} }},
        xaxis: {{
            title: {{ text: xCol, font: {{ size: 21 }} }},
            type: (chartType === 'box') ? 'category' : xScale,
            automargin: true,
            exponentformat: numFmt,
            showexponent: 'all',
            tickfont: {{ size: 17 }},
        }},
        yaxis: {{
            title: {{ text: yCol, font: {{ size: 21 }} }},
            type: (chartType === 'histogram') ? 'linear' : yScale,
            automargin: true,
            exponentformat: numFmt,
            showexponent: 'all',
            tickfont: {{ size: 17 }},
        }},
        legend: {{ orientation: 'v', x: 1.02, y: 1, font: {{ size: 17 }} }},
        margin: {{ l: 100, r: 280, t: 70, b: 80 }},
        hovermode: 'closest',
        template: 'plotly_white',
    }};

    Plotly.newPlot('chart', traces, layout, {{
        responsive: true,
        displayModeBar: true,
        modeBarButtonsToAdd: ['toggleSpikelines'],
    }});
}}

// 初回ロード時: カウント計算 + チャート描画
updateFilterCounts();
updateChart();
</script>
</body>
</html>"""
    return html


# ============================================================
# 3. 出力
# ============================================================
print("ダッシュボード生成中...")
html_content = build_dashboard_html()

with open(OUT_HTML, "w", encoding="utf-8") as f:
    f.write(html_content)

print(f"出力: {OUT_HTML}")
print(f"サイズ: {os.path.getsize(OUT_HTML):,} bytes")

try:
    webbrowser.open("file://" + OUT_HTML)
    print("ブラウザで開きました (失敗時は手動でファイルを開いてください)")
except Exception as e:
    print(f"ブラウザ起動失敗: {e}")
    print(f"手動で開いてください: file://{OUT_HTML}")
