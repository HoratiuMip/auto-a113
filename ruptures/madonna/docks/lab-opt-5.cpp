#include <bridge.hpp>
using namespace mdn;
using namespace a113;
using namespace glm;

struct Dock : dock_t {
public:
    Dock( void ) {
        f     = [] MDN_FNC_2D_L(double) {
            return 10.0*x1*x1 + 6.0*x2*x2 + 8.0*pow(x1,4)*pow(x2,4) + 24;
        };
        fd[0] = [] MDN_FNC_2D_L(double) {
            return 20.0*x1 + 32.0*pow(x1,3)*pow(x2,4);
        };
        fd[1] = [] MDN_FNC_2D_L(double) {
            return 12.0*x2 + 32.0*pow(x1,4)*pow(x2,3);
        };
        fdd[0] = [] MDN_FNC_2D_L(double) {
            return 20.0 + 96.0*x1*x1*pow(x2,4);
        };
        fdd[1] = [] MDN_FNC_2D_L(double) {
            return 128.0*pow(x1,3)*pow(x2,3);
        };
        fdd[2] = [] MDN_FNC_2D_L(double) {
            return 128.0*pow(x1,3)*pow(x2,3);
        };
        fdd[3] = [] MDN_FNC_2D_L(double) {
            return 12.0 + 96.0*pow(x1,4)*x2*x2;
        };

        constexpr double step = 0.01;
        grid.span_s( { { step, -1, 1 }, { step, -1, 1 } } );
        grid.apply( [ this ] ( double* x ) -> double { return f( x[0], x[1] ); } );
    }

public:
    inline static constexpr int METHOD_COUNT = 3;
    inline static const char* const METHODS[ METHOD_COUNT ] = {
        "Newton",
        "Steepest",
        "Conjugate"
    };
    enum Method_ {
        Method_Newton,
        Method_Steepest,
        Method_Conjugate
    };

public:
    mdn_0::fnc_2d_t< double >     f;
    mdn_0::fnc_2d_t< double >     fd[ 2 ];
    mdn_0::fnc_2d_t< double >     fdd[ 4 ];
    mdn_2::srf_grid_t< double >   grid;

    int                           step_count[ METHOD_COUNT ] = { 0 };
    double                        grad[ 2 ];
    double                        hess[ 4 ];
    dvec2                         x0{ 0.5, 0.5 };
    double                        s{ 1.0 };

public:
    void compute_gradient( dvec2 X ) {
        grad[0] = fd[0]( X.x, X.y );
        grad[1] = fd[1]( X.x, X.y );
    }

    void compute_hessian( dvec2 X ) {
        hess[0] = fdd[0]( X.x, X.y );
        hess[1] = fdd[1]( X.x, X.y );
        hess[2] = fdd[2]( X.x, X.y );
        hess[3] = fdd[3]( X.x, X.y );
    }

public:
    MDN_DOCK_NAME_FNC override { return "lab-opt-5"; }

    MDN_DOCK_GUIX_FNC override {
        bool open = true;
        if( ImGui::Begin( "Optimization Lab 5", &open, ImGuiWindowFlags_None ) ) {
            ImGui::SeparatorText( "f(x1,x2)" );
            ImGui::TextUnformatted( "10x1^2 + 6x2^2 + 8x1^4x2^4 + 24" );
            ImGui::Separator();

            ImPlot::PushColormap( ImPlotColormap_Viridis );
            if( ImPlot::BeginPlot( "##plt-f", {680, 680} ) ) {
                if( ImPlot::IsPlotHovered() && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
                    auto p = ImPlot::GetPlotMousePos();
                    x0.x = p.x; x0.y = p.y;
                }

                ImPlot::PlotHeatmap(
                    "##htm-f", grid.raw(), grid.n_of(1), grid.n_of(0),
                    grid.min(), grid.max(), nullptr, {-1,-1}, {1,1},
                    ImPlotHeatmapFlags_None
                );

                /* === Newton === */ {
                dvec2 xk = x0;
                for( int n = 1; n <= step_count[ Method_Newton ]; ++n ) {
                    compute_gradient( xk ); 
                    compute_hessian( xk );
                    MDN_ASSERT_OR( A113_OK == mdn_0::invm( hess, 2 ) ) break;

                    dvec2 xk1 = {
                        xk.x - hess[0]*grad[0] - hess[1]*grad[1],
                        xk.y - hess[2]*grad[0] - hess[3]*grad[1]
                    };

                    ImPlot::SetNextLineStyle( { 1,.36,0,1 }, 2 );
                    ImPlot::PlotLine( METHODS[ Method_Newton ], (double[2]){ xk.x, xk1.x }, (double[2]){ xk.y, xk1.y }, 2 );
                    if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Newton ] ) ) {
                        ImPlot::SetNextLineStyle( { 1,1,1,1 }, 2 );
                        ImPlot::PlotScatter( "", (double[2]){ xk.x, xk1.x }, (double[2]){ xk.y, xk1.y }, 2 );
                    }

                    xk = xk1;
                }
                }

                /* === Steepest === */ {
                dvec2 xk = x0;
                for( int n = 1; n <= step_count[ Method_Steepest ]; ++n ) {
                    compute_gradient( xk );
                    dvec2 dk = -dvec2{ grad[0], grad[1] }; dk /= length( dk );
                    
                    auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                        auto dxk = xk + dk*s_; return f( dxk.x, dxk.y );
                    } );

                    dvec2 xk1 = xk + dk*s;

                    ImPlot::SetNextLineStyle( { 1,0,.36,1 }, 2 );
                    ImPlot::PlotLine( METHODS[ Method_Steepest ], (double[2]){ xk.x, xk1.x }, (double[2]){ xk.y, xk1.y }, 2 );
                    if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Steepest ] ) ) {
                        ImPlot::SetNextLineStyle( { 1,1,1,1 }, 2 );
                        ImPlot::PlotScatter( "", (double[2]){ xk.x, xk1.x }, (double[2]){ xk.y, xk1.y }, 2 );
                    }

                    xk = xk1;
                }
                }

                /* === Conjugate === */ {
                dvec2  xk = x0;
                dvec2  dk = { 0, 0 };
                double bk = 0;
                compute_gradient( xk );
                for( int n = 1; n <= step_count[ Method_Conjugate ]; ++n ) {
                    dvec2 gk = dvec2{ grad[0], grad[1] };
                    
                    dk = -gk + dk*bk;

                    auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                        auto dxk = xk + dk*s_; return f( dxk.x, dxk.y );
                    } );

                    dvec2 xk1 = xk + dk*s;

                    ImPlot::SetNextLineStyle( { 1,.72,0,1 }, 2 );
                    ImPlot::PlotLine( METHODS[ Method_Conjugate ], (double[2]){ xk.x, xk1.x }, (double[2]){ xk.y, xk1.y }, 2 );
                    if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Conjugate ] ) ) {
                        ImPlot::SetNextLineStyle( { 1,1,1,1 }, 2 );
                        ImPlot::PlotScatter( "", (double[2]){ xk.x, xk1.x }, (double[2]){ xk.y, xk1.y }, 2 );
                    }

                    compute_gradient( xk1 );
                    dvec2 gk1 = dvec2{ grad[0], grad[1] };
                    
                    bk = pow(length(gk1),2) / pow(length(gk),2);

                    xk = xk1;
                }
                }

                ImPlot::EndPlot();
            }
        
            ImGui::SameLine();
            ImPlot::ColormapScale( "z", grid.min(), grid.max() );
            ImPlot::PopColormap();
            
            ImGui::SeparatorText( "Step count" );
            for( int i = 0x0; i < METHOD_COUNT; ++i ) {
                ImGui::PushID( i );
                ImGui::SliderInt( METHODS[ i ], step_count + i, 1, 100 );
                ImGui::PopID();
            }
            ImGui::Separator();
        } 
        ImGui::End();
        return open ? A113_OK : A113_ERR_TERMINATED; 
    }
};

struct Proxy : proxy_t {
public:
    Proxy( void )
    : proxy_t{
        "lab-opt-5",
        {},
        { 
        {   .text = "install",
            .fnc = [ this ] ( auto& stencil_ ) -> status_t {
                BridgE.install( a113::HVec< Dock >::make() );
                return A113_OK;
            }
        }
        }
    } {}
};

MDN_AUTO_INSTALL( Proxy );
