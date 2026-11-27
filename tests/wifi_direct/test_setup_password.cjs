const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const html = fs.readFileSync('firmware/core/jr_wifi_setup.html','utf8');
const script = html.split('<script>')[1].split('</script>')[0];
async function run(password) {
  const nodes = new Map();
  function node(id) {
    if (!nodes.has(id)) nodes.set(id, {value:'',textContent:'',type:'password',disabled:false,
      focus(){this.focused=true},setAttribute(k,v){this[k]=v},querySelectorAll(){return []},
      replaceChildren(){},append(){},classList:{remove(){},add(){}}});
    return nodes.get(id);
  }
  const posts=[];
  const context=vm.createContext({document:{getElementById:node,createElement:()=>node(Symbol())},
    URLSearchParams,Set,confirm:()=>true,fetch:async(url,options)=>{
      if(options?.method==='POST')posts.push({url,options});
      return {ok:true,json:async()=>({aps:[],profiles:[],ok:true,restarting:true})};
    }});
  vm.runInContext(script,context);
  const input=node('direct-password');input.value=password;input.oninput();
  assert.match(node('direct-count').textContent,new RegExp('^'+password.length+' caracteres'));
  node('direct-show').onclick();assert.equal(input.type,'text');
  node('direct-show').onclick();assert.equal(input.type,'password');
  await node('direct-on').onclick();
  return {posts,nodes};
}
(async()=>{
  for (const password of ['12345678','Abcd1234','a+b&c=12','x'.repeat(63)]) {
    const {posts,nodes}=await run(password);
    assert.equal(posts.length,1);
    assert.equal(posts[0].url,'/direct');
    assert.equal(new URLSearchParams(posts[0].options.body).get('pass'),password);
    assert.equal(nodes.get('direct-on')['aria-checked'],'true');
    assert.match(nodes.get('direct-status').textContent,/Reiniciando/);
  }
  for (const [password,message] of [['1234567',/7 caracteres/],['abcdé123',/posição 5/],['1234567\u200b',/posição 8/],['x'.repeat(64),/mais de 63/]]) {
    const {posts,nodes}=await run(password);
    assert.equal(posts.length,0);
    assert.match(nodes.get('direct-status').textContent,message);
    assert.equal(nodes.get('direct-password').value,password);
  }
  console.log('PASS: actual setup button submits valid passwords unchanged; length and invalid-character feedback; show/hide');
})().catch(error=>{console.error(error);process.exitCode=1});
