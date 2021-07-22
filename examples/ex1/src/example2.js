//@namespace example
//@template template_test.html
//@style template_test.css
function ex2() {
	var x = loadTemplate("template_test");
	document.body.appendChild(x);
}

example.test2=ex2;
