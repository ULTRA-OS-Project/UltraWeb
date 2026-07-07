// UltraWeb/tests/HTMLGeneratorTests.cpp
// Unit tests for the crawler/fallback HTML generator
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "../include/UltraWebBundler.h"
#include "../include/UltraWebHTMLGenerator.h"

#include <iostream>
#include <string>

using namespace UltraWeb;
using namespace UltraWeb::Server;

namespace {

void PrintTestResult(const std::string& testName, bool passed) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << testName << std::endl;
}

void PrintSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

bool Contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

const char* kUcml = R"(
    <div id="app" class="container">
        <text id="title" class="heading">Product Overview</text>
        <text id="body">Plain description text</text>
        <image id="logo" src="/img/logo.png" alt="Company logo" />
        <button id="cta" class="btn">Order now</button>
        <input id="email" placeholder="Your email" />
        <text id="secret" visible="false">Hidden admin note</text>
    </div>
)";

const char* kCss = ".heading { font-size: 24px; }";

HTMLPageMeta TestMeta() {
    HTMLPageMeta meta;
    meta.title = "Product <Overview>";
    meta.description = "All about the \"product\"";
    meta.canonicalUrl = "https://example.com/product";
    meta.ogImage = "https://example.com/og.png";
    meta.jsonLd = R"({"@context":"https://schema.org","@type":"Product"})";
    return meta;
}

// ============================================================================
// TESTS
// ============================================================================

bool TestHTML_Escape() {
    return HTMLGenerator::Escape("<a href=\"x\">&'</a>") ==
           "&lt;a href=&quot;x&quot;&gt;&amp;&#39;&lt;/a&gt;";
}

bool TestHTML_SemanticMapping() {
    HTMLGenerator generator;
    auto page = generator.GenerateFromSources(kUcml, kCss, TestMeta());
    if (!page.success) {
        std::cout << "  " << page.error << std::endl;
        return false;
    }
    const std::string& html = page.html;

    // Heading class renders as h1; plain text as p
    if (!Contains(html, "<h1 id=\"title\"")) return false;
    if (!Contains(html, "Product Overview</h1>")) return false;
    if (!Contains(html, "<p id=\"body\">Plain description text</p>")) return false;
    // Button, image with src/alt, input with placeholder
    if (!Contains(html, "<button id=\"cta\" class=\"btn\">Order now</button>")) return false;
    if (!Contains(html, "<img id=\"logo\"")) return false;
    if (!Contains(html, "src=\"/img/logo.png\"")) return false;
    if (!Contains(html, "alt=\"Company logo\"")) return false;
    if (!Contains(html, "placeholder=\"Your email\"")) return false;
    // Container div with id/class
    if (!Contains(html, "<div id=\"app\" class=\"container\">")) return false;
    return true;
}

bool TestHTML_HiddenElementsOmitted() {
    HTMLGenerator generator;
    auto page = generator.GenerateFromSources(kUcml, kCss, TestMeta());
    // visible="false" content must not leak to crawlers
    return page.success && !Contains(page.html, "Hidden admin note");
}

bool TestHTML_MetadataAndParity() {
    HTMLGenerator generator;
    auto page = generator.GenerateFromSources(kUcml, kCss, TestMeta());
    if (!page.success) return false;
    const std::string& html = page.html;

    if (!Contains(html, "<title>Product &lt;Overview&gt;</title>")) return false;
    if (!Contains(html, "name=\"description\" content=\"All about the &quot;product&quot;\"")) return false;
    if (!Contains(html, "<link rel=\"canonical\" href=\"https://example.com/product\">")) return false;
    if (!Contains(html, "property=\"og:title\"")) return false;
    if (!Contains(html, "property=\"og:image\" content=\"https://example.com/og.png\"")) return false;
    if (!Contains(html, "name=\"twitter:card\"")) return false;
    if (!Contains(html, "application/ld+json")) return false;
    if (!Contains(html, "\"@type\":\"Product\"")) return false;
    if (!Contains(html, "<style>.heading { font-size: 24px; }</style>")) return false;
    if (!Contains(html, "<html lang=\"en\">")) return false;
    return true;
}

bool TestHTML_TextIsEscaped() {
    // Special characters in element text and attributes must be escaped in
    // the HTML output (the UCML parser stores text verbatim)
    HTMLGenerator generator;
    auto page = generator.GenerateFromSources(
        "<div id=\"app\"><text id=\"t\">Tom & Jerry's \"special\" offer</text></div>",
        "", TestMeta());
    if (!page.success) return false;
    return Contains(page.html,
                    "Tom &amp; Jerry&#39;s &quot;special&quot; offer");
}

bool TestHTML_LoaderScriptToggle() {
    HTMLGenerator generator;
    auto withLoader = generator.GenerateFromSources(kUcml, "", TestMeta());
    if (!withLoader.success ||
        !Contains(withLoader.html, "/uw-runtime.js")) return false;
    if (!Contains(withLoader.html, "/app.ucpkg")) return false;

    generator.SetIncludeLoaderScript(false);
    auto without = generator.GenerateFromSources(kUcml, "", TestMeta());
    return without.success && !Contains(without.html, "<script>(function()");
}

bool TestHTML_FromPackageMatchesFromSources() {
    // Content parity: the page generated from the compiled package equals
    // the page generated from the sources it was compiled from
    HTMLGenerator generator;
    auto fromSources = generator.GenerateFromSources(kUcml, kCss, TestMeta());

    PackageBundler bundler;
    bundler.SetUIFromSource(kUcml);
    BundleResult bundle = bundler.Bundle();
    if (!bundle.success) return false;
    auto fromPackage = generator.GenerateFromPackage(bundle.data, TestMeta(), kCss);

    if (!fromSources.success || !fromPackage.success) return false;
    if (fromSources.html != fromPackage.html) {
        std::cout << "  outputs differ" << std::endl;
        return false;
    }
    return true;
}

bool TestHTML_SizeBudget() {
    HTMLGenerator generator;
    auto page = generator.GenerateFromSources(kUcml, kCss, TestMeta());
    if (!page.success) return false;
    std::cout << "  page size: " << page.html.size() << " bytes" << std::endl;
    return page.html.size() < 15 * 1024;  // spec: < 15KB per page
}

bool TestHTML_SitemapAndRobots() {
    std::string sitemap = HTMLGenerator::GenerateSitemap(
        {"https://example.com/", "https://example.com/products?id=1&x=2"});
    if (!Contains(sitemap, "<?xml version=\"1.0\"")) return false;
    if (!Contains(sitemap, "<loc>https://example.com/</loc>")) return false;
    // XML-escaped query string
    if (!Contains(sitemap, "id=1&amp;x=2")) return false;

    std::string robots =
        HTMLGenerator::GenerateRobotsTxt("https://example.com/sitemap.xml");
    return Contains(robots, "User-agent: *") &&
           Contains(robots, "Sitemap: https://example.com/sitemap.xml");
}

bool TestHTML_LinksRenderAsAnchors() {
    HTMLGenerator generator;
    auto page = generator.GenerateFromSources(
        "<div id=\"app\"><text id=\"l\" href=\"/about\">About us</text></div>",
        "", TestMeta());
    if (!page.success) return false;
    return Contains(page.html, "<a id=\"l\" href=\"/about\">About us</a>");
}

} // namespace

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    PrintSeparator();
    std::cout << "UltraWeb HTML Generator Test Suite (Crawler & Fallback)\n";
    PrintSeparator();

    int passed = 0;
    int failed = 0;
    auto run = [&](const char* name, bool ok) {
        if (ok) passed++; else failed++;
        PrintTestResult(name, ok);
    };

    std::cout << "\n[HTML Generator Tests]\n";
    run("Escaping", TestHTML_Escape());
    run("Semantic Mapping", TestHTML_SemanticMapping());
    run("Hidden Elements Omitted", TestHTML_HiddenElementsOmitted());
    run("Metadata & Head Parity", TestHTML_MetadataAndParity());
    run("Text Injection Escaped", TestHTML_TextIsEscaped());
    run("Loader Script Toggle", TestHTML_LoaderScriptToggle());
    run("Package/Source Parity", TestHTML_FromPackageMatchesFromSources());
    run("Size Budget (<15KB)", TestHTML_SizeBudget());
    run("Sitemap & robots.txt", TestHTML_SitemapAndRobots());
    run("Links Render As Anchors", TestHTML_LinksRenderAsAnchors());

    std::cout << "\n";
    PrintSeparator();
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    PrintSeparator();
    std::cout << "\n";

    return failed > 0 ? 1 : 0;
}
