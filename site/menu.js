// Monades site: the menu button on narrow screens, and the light / dark switch (remembered when storage allows).
(function(){
  var root=document.documentElement;
  try{var t=localStorage.getItem('monades-theme');if(t)root.setAttribute('data-theme',t);}catch(e){}
  var btn=document.getElementById('theme');
  if(btn)btn.addEventListener('click',function(){
    var next=root.getAttribute('data-theme')==='light'?'dark':'light'; // deep space unless switched to daylight
    root.setAttribute('data-theme',next);try{localStorage.setItem('monades-theme',next);}catch(e){}
  });
  var nav=document.querySelector('nav.menu'),mb=nav&&nav.querySelector('.menu-btn');
  if(mb)mb.addEventListener('click',function(){var open=nav.classList.toggle('open');mb.setAttribute('aria-expanded',open?'true':'false');});
  // A link to a folded list (#gammes, #lots) unfolds it.
  document.querySelectorAll('details[id]').forEach(function(d){
    if(location.hash==='#'+d.id)d.open=true;
    document.querySelectorAll('a[href$="#'+d.id+'"]').forEach(function(a){a.addEventListener('click',function(){d.open=true;});});
  });
})();
