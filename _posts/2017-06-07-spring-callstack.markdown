---
layout: post
title:  "spring callstack"
date:   2017-06-07 21:00:00 +0800
categories: spring
---

# spring callstack

这个callstack记录了spring在收到一个request之后是怎样分发给对应的controller的。Spring版本是org.springframework:spring-core:4.2.7。

[Source code]( https://github.com/xiaocairush/demos/tree/master/SpringGuides/gs-rest-service)
[breakpoint](https://github.com/xiaocairush/demos/blob/master/SpringGuides/gs-rest-service/complete/src/main/java/hello/GreetingController.java#L16)


## f0 hello.GreetingController#greeting

最终将请求127.0.0.1:8080/greeting分发给这个controller的greeting方法，除此之外没啥好说的。

## f1 sun.reflect.NativeMethodAccessorImpl#invoke0

```
private static native Object invoke0(Method var0, Object var1, Object[] var2);
```
native方法，注意参数列表即可。其它暂不讨论。

## f2 sun.reflect.NativeMethodAccessorImpl#invoke

```
public Object invoke(Object var1, Object[] var2) throws IllegalArgumentException, InvocationTargetException {
		// step 0. 增加调用此方法的计数，此处应该是安全检查，与spring无关，不展开讨论,注意可能会抛出异常即可
        if(++this.numInvocations > ReflectionFactory.inflationThreshold() && !ReflectUtil.isVMAnonymousClass(this.method.getDeclaringClass())) {
            MethodAccessorImpl var3 = (MethodAccessorImpl)(new MethodAccessorGenerator()).generateMethod(this.method.getDeclaringClass(), this.method.getName(), this.method.getParameterTypes(), this.method.getReturnType(), this.method.getExceptionTypes(), this.method.getModifiers());
            this.parent.setDelegate(var3);
        }
		// step 1. 通过这行调用f1
        return invoke0(this.method, var1, var2);
    }
```

## f3 sun.reflect.DelegatingMethodAccessorImpl#invoke

```
// 不是spring package的类，不展开讨论，注意一下函数签名即可
public Object invoke(Object var1, Object[] var2) throws IllegalArgumentException, InvocationTargetException {
        return this.delegate.invoke(var1, var2);
    }
```

## f4 java.lang.reflect.Method#invoke

平常使用的[Method](https://docs.oracle.com/javase/tutorial/reflect/member/methodInvocation.html)类，与Spring无关。

```
// obj的内容是GreetingController, args里面只有一个“World”字符串
public Object invoke(Object obj, Object... args)
{
	...
}
```

可以看出hello.GreetingController#greeting对应了spring中的一个Method对象，通过反射来触发调用greeting函数。

## f5 org.springframework.web.method.support.InvocableHandlerMethod#doInvoke

 该类位于spring-web模块。
 
```
protected Method getBridgedMethod() {
		return this.bridgedMethod;
	}
	
// args里面只有一个“World”字符串
protected Object doInvoke(Object... args) throws Exception {
		// spring支持将greeting方法声明为private
		ReflectionUtils.makeAccessible(getBridgedMethod());
		try {
			// 此处调用f4
			return getBridgedMethod().invoke(getBean(), args);
		}
		catch (IllegalArgumentException ex) {
			...
		}
		catch (InvocationTargetException ex) {
			...
		}
```

异常情况不展开讨论，因为我们只关注spring核心逻辑。因此org.springframework.web.method.support.InvocableHandlerMethod#doInvoke只是java.lang.reflect.Method#invoke的一个简单的wrapper，处理一些反射时出现的异常情况，无他。

## f6 org.springframework.web.method.support.InvocableHandlerMethod#invokeForRequest

 该类位于spring-web模块。

```
// 入参的内容
// ServletWebRequest: uri=/greeting;client=127.0.0.1
// ModelAndViewContainer: View is [null]; default model {}
// 可以发现
public Object invokeForRequest(NativeWebRequest request, ModelAndViewContainer mavContainer,
			Object... providedArgs) throws Exception {

		// step 0. 从NativeWebRequest和ModelAndView中提取参数，关注主要逻辑，暂不看细节实现
		Object[] args = getMethodArgumentValues(request, mavContainer, providedArgs);
		
		// step 1. 打开trace log会有日志，protected final Log logger = LogFactory.getLog(getClass());
		if (logger.isTraceEnabled()) {
			StringBuilder sb = new StringBuilder("Invoking [");
			sb.append(getBeanType().getSimpleName()).append(".");
			sb.append(getMethod().getName()).append("] method with arguments ");
			sb.append(Arrays.asList(args));
			logger.trace(sb.toString());
		}
		
		// step 2. 此处调用f5
		Object returnValue = doInvoke(args);
		if (logger.isTraceEnabled()) {
			logger.trace("Method [" + getMethod().getName() + "] returned [" + returnValue + "]");
		}
		return returnValue;
	}
```

invokeForRequest只是根据request等入参来获取greeting方法参数列表中的参数，然后通过反射来调用greeting方法。

## f7 org.springframework.web.servlet.mvc.method.annotation.ServletInvocableHandlerMethod#invokeAndHandle

 该类位于spring-web模块。
 
```
public void invokeAndHandle(ServletWebRequest webRequest,
			ModelAndViewContainer mavContainer, Object... providedArgs) throws Exception {

		// step 0. 此处调用f6来获取此次request的返回值
		Object returnValue = invokeForRequest(webRequest, mavContainer, providedArgs);
		// 设置request的响应状态，在这次请求中，由于响应状态是null，实际上webRequest没有设置响应状态。
		// todo: 看一下greeting抛出exception时是怎样处理的。
		setResponseStatus(webRequest);

		if (returnValue == null) {
			if (isRequestNotModified(webRequest) || hasResponseStatus() || mavContainer.isRequestHandled()) {
				mavContainer.setRequestHandled(true);
				return;
			}
		}
		else if (StringUtils.hasText(this.responseReason)) {
			mavContainer.setRequestHandled(true);
			return;
		}

		mavContainer.setRequestHandled(false);
		try {
			this.returnValueHandlers.handleReturnValue(
					returnValue, getReturnValueType(returnValue), mavContainer, webRequest);
		}
		catch (Exception ex) {
			if (logger.isTraceEnabled()) {
				logger.trace(getReturnValueHandlingErrorMessage("Error handling return value", returnValue), ex);
			}
			throw ex;
		}
	}
```
