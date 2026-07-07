// UltraWeb/core/UltraWebHTMLGenerator.cpp
// Crawler & fallback HTML generation implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "../include/UltraWebHTMLGenerator.h"

#include "../include/UltraWebBundler.h"
#include "../include/UltraWebUICompiler.h"
#include "../runtime/UltraWebRuntime.h"

#include <sstream>

namespace UltraWeb {
namespace Server {

namespace {

using Runtime::RuntimeElement;
using Runtime::UltraWebRuntime;

struct TagInfo {
    const char* tag;
    bool isVoid;         // no closing tag (img, input)
};

// Semantic mapping from UCB element types to HTML tags. Interactive
// widget internals (sliders, tabs, ...) degrade to generic containers -
// crawlers need the content, not the widget chrome.
TagInfo TagFor(UCBElementType type) {
    switch (type) {
        case UCBElementType::Text:      return {"p", false};
        case UCBElementType::Button:    return {"button", false};
        case UCBElementType::Input:     return {"input", true};
        case UCBElementType::TextArea:  return {"textarea", false};
        case UCBElementType::Checkbox:  return {"input", true};
        case UCBElementType::Radio:     return {"input", true};
        case UCBElementType::Select:    return {"select", false};
        case UCBElementType::Image:     return {"img", true};
        case UCBElementType::Video:     return {"video", false};
        case UCBElementType::Audio:     return {"audio", false};
        case UCBElementType::List:      return {"ul", false};
        case UCBElementType::Table:     return {"table", false};
        default:                        return {"div", false};
    }
}

// Text elements with heading-ish class names render as headings so the
// document outline survives for crawlers
const char* HeadingTagFor(const RuntimeElement& el) {
    for (const auto& cls : el.classNames) {
        if (cls == "h1" || cls == "title" || cls == "heading") return "h1";
        if (cls == "h2" || cls == "subtitle" || cls == "subheading") return "h2";
        if (cls == "h3") return "h3";
    }
    return nullptr;
}

void RenderElement(UltraWebRuntime& runtime, const RuntimeElement& el,
                   std::ostringstream& out, int depth) {
    if (!el.visible) return;  // parity: hidden in the app, absent for crawlers
    if (depth > 64) return;   // cycle/depth guard

    TagInfo info = TagFor(el.type);
    const char* tag = info.tag;
    if (el.type == UCBElementType::Text) {
        if (const char* heading = HeadingTagFor(el)) tag = heading;
    }

    // Links: any element carrying an Href renders as an anchor so crawlers
    // can follow it
    std::string href = el.GetStringProp(UCBPropertyId::Href);
    if (!href.empty()) {
        tag = "a";
        info.isVoid = false;
    }

    out << "<" << tag;
    if (!el.id.empty()) out << " id=\"" << HTMLGenerator::Escape(el.id) << "\"";
    if (!el.classNames.empty()) {
        out << " class=\"";
        for (size_t i = 0; i < el.classNames.size(); i++) {
            if (i) out << ' ';
            out << HTMLGenerator::Escape(el.classNames[i]);
        }
        out << "\"";
    }
    if (!href.empty()) out << " href=\"" << HTMLGenerator::Escape(href) << "\"";

    if (el.type == UCBElementType::Image) {
        out << " src=\"" << HTMLGenerator::Escape(
                   el.GetStringProp(UCBPropertyId::Src)) << "\"";
        out << " alt=\"" << HTMLGenerator::Escape(
                   el.GetStringProp(UCBPropertyId::Alt)) << "\"";
    }
    if (el.type == UCBElementType::Input || el.type == UCBElementType::Checkbox ||
        el.type == UCBElementType::Radio) {
        if (el.type == UCBElementType::Checkbox) out << " type=\"checkbox\"";
        else if (el.type == UCBElementType::Radio) out << " type=\"radio\"";
        std::string placeholder = el.GetStringProp(UCBPropertyId::Placeholder);
        if (!placeholder.empty()) {
            out << " placeholder=\"" << HTMLGenerator::Escape(placeholder) << "\"";
        }
        std::string value = el.GetStringProp(UCBPropertyId::Value);
        if (!value.empty()) {
            out << " value=\"" << HTMLGenerator::Escape(value) << "\"";
        }
    }
    if (!el.enabled) out << " disabled";

    if (info.isVoid) {
        out << ">";
        return;
    }
    out << ">";

    if (!el.textContent.empty()) {
        out << HTMLGenerator::Escape(el.textContent);
    }
    for (uint16_t childId : el.childIds) {
        const RuntimeElement* child = runtime.GetElement(childId);
        if (child) RenderElement(runtime, *child, out, depth + 1);
    }
    out << "</" << tag << ">";
}

std::string LoaderScript(const std::string& runtimeUrl,
                         const std::string& packageUrl) {
    // Progressive enhancement (spec: Serving Strategy, Mode A): try to
    // load the UltraWeb runtime; when it is not deployed the request
    // fails silently and the static HTML simply remains.
    return "<script>(function(){if(!('WebAssembly' in window))return;"
           "var s=document.createElement('script');s.src='" + runtimeUrl + "';"
           "s.async=true;s.onload=function(){if(window.UltraWebBoot)"
           "window.UltraWebBoot('" + packageUrl + "');};"
           "s.onerror=function(){};document.head.appendChild(s);})();"
           "</script>";
}

} // namespace

std::string HTMLGenerator::Escape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '&':  out += "&amp;"; break;
            case '<':  out += "&lt;"; break;
            case '>':  out += "&gt;"; break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default:   out += c;
        }
    }
    return out;
}

HTMLGenerateResult HTMLGenerator::GenerateFromSources(const std::string& ucml,
                                                      const std::string& css,
                                                      const HTMLPageMeta& meta) {
    HTMLGenerateResult result;

    UICompiler compiler;
    UICompilationResult compiled = compiler.Compile(ucml);
    if (!compiled.success) {
        result.error = "UCML compilation failed";
        for (const auto& e : compiled.errors) result.error += ": " + e;
        return result;
    }

    PackageBundler bundler;
    bundler.SetUISection(compiled.data);
    BundleResult bundle = bundler.Bundle();
    if (!bundle.success) {
        result.error = "bundling failed";
        return result;
    }
    return GenerateFromPackage(bundle.data, meta, css);
}

HTMLGenerateResult HTMLGenerator::GenerateFromPackage(
    const std::vector<uint8_t>& ucpkg, const HTMLPageMeta& meta,
    const std::string& css) {
    HTMLGenerateResult result;

    UltraWebRuntime runtime;
    auto load = runtime.LoadPackage(ucpkg);
    if (!load.success) {
        result.error = "package load failed: " + load.error;
        return result;
    }

    std::ostringstream body;
    const RuntimeElement* root = runtime.GetRootElement();
    if (root) {
        RenderElement(runtime, *root, body, 0);
    }

    std::ostringstream out;
    out << "<!doctype html>\n<html lang=\""
        << Escape(meta.language.empty() ? "en" : meta.language) << "\">\n<head>\n"
        << "<meta charset=\"utf-8\">\n"
        << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        << "<meta name=\"generator\" content=\"UltraWeb HTMLGenerator\">\n"
        << "<title>" << Escape(meta.title) << "</title>\n";

    if (!meta.description.empty()) {
        out << "<meta name=\"description\" content=\""
            << Escape(meta.description) << "\">\n";
    }
    if (!meta.canonicalUrl.empty()) {
        out << "<link rel=\"canonical\" href=\"" << Escape(meta.canonicalUrl)
            << "\">\n";
    }

    // Open Graph + Twitter Card (spec: link previews are HTML-only bots)
    out << "<meta property=\"og:type\" content=\"website\">\n"
        << "<meta property=\"og:title\" content=\"" << Escape(meta.title)
        << "\">\n";
    if (!meta.description.empty()) {
        out << "<meta property=\"og:description\" content=\""
            << Escape(meta.description) << "\">\n";
    }
    if (!meta.canonicalUrl.empty()) {
        out << "<meta property=\"og:url\" content=\""
            << Escape(meta.canonicalUrl) << "\">\n";
    }
    if (!meta.ogImage.empty()) {
        out << "<meta property=\"og:image\" content=\"" << Escape(meta.ogImage)
            << "\">\n";
    }
    out << "<meta name=\"twitter:card\" content=\"summary\">\n";

    if (!meta.jsonLd.empty()) {
        out << "<script type=\"application/ld+json\">" << meta.jsonLd
            << "</script>\n";
    }
    if (!css.empty()) {
        out << "<style>" << css << "</style>\n";
    }
    out << "</head>\n<body>\n";
    out << body.str();
    out << "\n";
    if (includeLoader) {
        out << LoaderScript(runtimeUrl, packageUrl) << "\n";
    }
    out << "</body>\n</html>\n";

    result.html = out.str();
    result.success = true;
    return result;
}

std::string HTMLGenerator::GenerateSitemap(
    const std::vector<std::string>& urls) {
    std::ostringstream out;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n";
    for (const auto& url : urls) {
        out << "  <url><loc>" << Escape(url) << "</loc></url>\n";
    }
    out << "</urlset>\n";
    return out.str();
}

std::string HTMLGenerator::GenerateRobotsTxt(const std::string& sitemapUrl) {
    std::string out = "User-agent: *\nAllow: /\n";
    if (!sitemapUrl.empty()) out += "Sitemap: " + sitemapUrl + "\n";
    return out;
}

} // namespace Server
} // namespace UltraWeb
