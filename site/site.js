(function () {
  "use strict";

  var root = document.documentElement;
  var themeBtn = document.querySelector("button.theme");

  function currentTheme() {
    var attr = root.getAttribute("data-theme");
    if (attr === "light" || attr === "dark") return attr;
    if (window.matchMedia && window.matchMedia("(prefers-color-scheme: light)").matches) {
      return "light";
    }
    return "dark";
  }

  function updateThemeButton(theme) {
    if (!themeBtn) return;
    var next = theme === "light" ? "dark" : "light";
    themeBtn.setAttribute("aria-label", "Switch to the " + next + " theme");
    themeBtn.setAttribute("title", "Switch to " + next + " theme");
  }

  updateThemeButton(currentTheme());

  if (themeBtn) {
    themeBtn.addEventListener("click", function () {
      var next = currentTheme() === "light" ? "dark" : "light";
      root.setAttribute("data-theme", next);
      updateThemeButton(next);
      try {
        localStorage.setItem("coriolis-theme", next);
      } catch (e) {}
    });
  }

  // Scene category filters
  var filterTabs = document.querySelectorAll(".tab-btn[data-filter]");
  var shotCards = document.querySelectorAll(".shot-card[data-cat]");

  filterTabs.forEach(function (tab) {
    tab.addEventListener("click", function () {
      var wanted = tab.dataset.filter;
      filterTabs.forEach(function (t) {
        t.setAttribute("aria-pressed", t === tab ? "true" : "false");
      });
      shotCards.forEach(function (card) {
        card.hidden = wanted !== "all" && card.dataset.cat !== wanted;
      });
    });
  });

  // Card "Run in emulator" buttons
  var jumpButtons = document.querySelectorAll("[data-jump-scene]");
  var trySection = document.getElementById("try");
  jumpButtons.forEach(function (btn) {
    btn.addEventListener("click", function () {
      var idx = parseInt(btn.dataset.jumpScene, 10);
      if (trySection) {
        trySection.scrollIntoView({ behavior: "smooth", block: "start" });
      }
      if (typeof window.coriolisJumpTo === "function") {
        window.coriolisJumpTo(idx);
      }
    });
  });
})();
