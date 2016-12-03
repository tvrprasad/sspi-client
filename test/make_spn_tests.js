const MakeSpn = require('../src_js/make_spn.js');

exports.makeSpnPort = function(test) {
  const spn = MakeSpn.makeSpn('MSSQLSvc', 'www.example.com', 1433);
  test.strictEqual(spn, 'MSSQLSvc/www.example.com:1433');
  test.done();
}

exports.makeSpnInstanceName = function(test) {
  const spn = MakeSpn.makeSpn('MSSQLSvc', 'www.example.com', 'myinstance');
  test.strictEqual(spn, 'MSSQLSvc/www.example.com:myinstance');
  test.done();
}
