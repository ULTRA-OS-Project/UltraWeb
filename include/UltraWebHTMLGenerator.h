// UltraWeb/include/UltraWebHTMLGenerator.h
// Crawler & fallback HTML generation (spec: Crawler & Fallback Rendering)
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Emits plain, dependency-free semantic HTML from the same UCML/CSS
// sources (or a compiled .ucpkg) that produce the binary bundles, so
// content parity between the binary app and the crawler pages holds by
// construction. Pages carry canonical link, description and Open Graph /
// Twitter Card metadata, plus optional JSON-LD. With the loader script
// enabled the page doubles as the Mode-A "HTML-first" response: capable
// browsers fetch the UltraWeb runtime and swap the live app in place,
// crawlers and no-JS clients keep the HTML.
//
// Site-level artifacts (sitemap.xml, robots.txt) are provided as
// static helpers. Interactive/per-user views are not generated - only
// publicly reachable content routes (see the spec's non-goals).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace UltraWeb {
namespace Server {

struct HTMLPageMeta {
    std::string title;             // <title> and og:title
    std::string description;       // meta description and og:description
    std::string canonicalUrl;      // rel=canonical + og:url (empty = omitted)
    std::string language = "en";   // <html lang>
    std::string ogImage;           // og:image (empty = omitted)
    std::string jsonLd;            // raw JSON-LD object (empty = omitted)
};

struct HTMLGenerateResult {
    bool success = false;
    std::string error;
    std::string html;
};

class HTMLGenerator {
public:
    // Mode-A loader snippet: page tries to load the UltraWeb runtime and
    // swap in the live app; crawlers/no-JS clients keep the static HTML.
    void SetIncludeLoaderScript(bool include) { includeLoader = include; }
    void SetPackageUrl(const std::string& url) { packageUrl = url; }
    void SetRuntimeUrl(const std::string& url) { runtimeUrl = url; }

    // Generates the page from UCML + CSS sources (build-time path used by
    // uwc --emit-html and the dev server)
    HTMLGenerateResult GenerateFromSources(const std::string& ucml,
                                           const std::string& css,
                                           const HTMLPageMeta& meta);

    // Generates the page from a compiled .ucpkg (parity with what is
    // served). The binary style section is not decompiled; pass the CSS
    // source when styled fallback rendering is wanted.
    HTMLGenerateResult GenerateFromPackage(const std::vector<uint8_t>& ucpkg,
                                           const HTMLPageMeta& meta,
                                           const std::string& css = "");

    // ===== SITE-LEVEL ARTIFACTS =====

    static std::string GenerateSitemap(const std::vector<std::string>& urls);
    static std::string GenerateRobotsTxt(const std::string& sitemapUrl);

    // HTML text escaping (exposed for tests)
    static std::string Escape(const std::string& text);

private:
    bool includeLoader = true;
    std::string packageUrl = "/app.ucpkg";
    std::string runtimeUrl = "/uw-runtime.js";
};

} // namespace Server
} // namespace UltraWeb
