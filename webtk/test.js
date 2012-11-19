var idSequence = 0;

function Button(caption) {
    this.private = new Object();
    this.private.id = idSequence++;
    this.private.node = document.createElement('input');
    this.private.node.type = 'button';
    this.private.node.value = caption || ('Button ' + this.private.id);
}

Button.prototype = {
    get node() {
        return this.private.node;
    },
    set node(x) {
        this.private.node = x;
    },
    get onclick() {
        return this.private.onclick;
    },
    set onclick(f) {
        this.private.node.removeEventListener('click', this.private.onclick, false);
        this.private.onclick = f;
        this.private.node.addEventListener('click', this.private.onclick, false);
    },
};

function onclick(event) {
    alert('Hello!');
}

function init() {
    var button = new Button('Push Me');
    button.onclick = onclick;
    document.body.appendChild(button.node);
}
