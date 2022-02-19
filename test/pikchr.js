
function pikchrSourceToggle(ev){
  if (ev.altKey || ev.ctrlKey || ev.metaKey){
    this.classList.toggle('source');
    ev.stopPropagation();
    ev.preventDefault();
  }
}

document.querySelectorAll("svg.pikchr").forEach((e) => {
  const wrapper = e.parentNode.parentNode;
  const srcNode = parent ? e.parentNode.nextElementSibling : undefined;
  if (wrapper && srcNode && srcNode.classList.contains('pikchr-src')) {
    wrapper.addEventListener('click', pikchrSourceToggle, false);
  }
});
