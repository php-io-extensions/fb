<?php

namespace Zephir\Optimizers\FunctionCall;

use Zephir\Call;
use Zephir\CompilationContext;
use Zephir\CompiledExpression;
use Zephir\Exception\CompilerException;
use Zephir\Optimizers\OptimizerAbstract;

class FbPresentOptimizer extends OptimizerAbstract
{
    public function optimize(array $expression, Call $call, CompilationContext $context)
    {
        if (!isset($expression['parameters']) || count($expression['parameters']) !== 1) {
            throw new CompilerException("'fb_present' requires exactly one parameter", $expression);
        }
        $resolvedParams = $call->getReadOnlyResolvedParams($expression['parameters'], $context, $expression);
        $context->headersManager->add('src/fb-api');
        $context->codePrinter->output('fb_api_present(' . implode(', ', $resolvedParams) . ');');

        return new CompiledExpression('null', null, $expression);
    }
}
